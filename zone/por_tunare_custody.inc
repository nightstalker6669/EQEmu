// Included in the original Give writer's private namespace.
namespace Tunare = EQEmu::SharedCore::PorTunare;

const Tunare::Route* OfferedTunare(Client& client, NPC& npc)
{
    if (!zone || zone->GetInstanceVersion() != 0) return nullptr;
    const auto offers = OriginalTradeOffer::Capture(client);
    for (const auto& route : Tunare::Routes) {
        if (!Tunare::Matches(route, zone->GetZoneID(), zone->GetInstanceID(),
                npc.GetNPCTypeID(), npc.GetSpawnPointID())) continue;
        if (std::ranges::any_of(offers, [&](const auto& offer) {
                return offer.present && Tunare::Input(route, offer.item_id);
            })) return &route;
    }
    return nullptr;
}

bool TunareCompleted(Database& db, uint32_t character, uint32_t task, uint64_t run = 0)
{
    return Q(db, fmt::format(
        "SELECT t.id FROM completed_shared_tasks t JOIN completed_shared_task_members m "
        "ON m.shared_task_id=t.id WHERE t.task_id={} AND m.character_id={} "
        "AND t.completion_time IS NOT NULL {} LIMIT 1 FOR UPDATE",
        task, character, run ? fmt::format("AND t.id={}", run) : "")).RowCount() == 1;
}

DynamicZone* CurrentTunareRaid()
{
    if (!zone || zone->GetZoneID() != Tunare::Shrine || zone->GetInstanceVersion() != 22 ||
        !zone->GetInstanceID()) return nullptr;
    auto* dz = zone->GetDynamicZone();
    return dz && dz->GetType() == DynamicZoneType::Mission && dz->IsCurrentZoneDz() &&
        !dz->IsExpired() && dz->GetUUID().size() == 36 ? dz : nullptr;
}

struct TunareGuardianLoot {
    Corpse& corpse;
    uint16_t slot;
};

uint64_t LockTunareGuardian(Database& db, Client& client, const TunareGuardianLoot& loot)
{
    auto* dz = CurrentTunareRaid();
    const auto who = client.CharacterID();
    if (!dz || !dz->HasMember(who) || !loot.corpse.IsNPCCorpse() ||
        loot.corpse.GetNPCTypeID() != Tunare::Guardian ||
        loot.corpse.GetEntityVariable(Tunare::GuardianTag) != dz->GetUUID() ||
        loot.corpse.GetItemIDBySlot(loot.slot) != Tunare::Chalice) return 0;
    auto rows = Q(db, fmt::format(
        "SELECT t.id FROM completed_shared_tasks t "
        "JOIN completed_shared_task_members m ON m.shared_task_id=t.id AND m.character_id={0} "
        "JOIN completed_shared_task_activity_state a ON a.shared_task_id=t.id AND a.activity_id=2 "
        "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=t.id "
        "JOIN dynamic_zones d ON d.id=sd.dynamic_zone_id "
        "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=d.id AND dm.character_id={0} "
        "JOIN instance_list i ON i.id=d.instance_id "
        "WHERE t.task_id={1} AND t.completion_time IS NOT NULL AND a.completed_time IS NOT NULL "
        "AND a.done_count=1 AND d.id={2} AND d.uuid='{3}' AND d.type=4 AND i.id={4} "
        "AND i.zone=379 AND i.version=22 FOR UPDATE",
        who, Tunare::Raid, dz->GetID(), db.Escape(dz->GetUUID()), dz->GetInstanceID()));
    return rows.RowCount() == 1 ? N(rows.begin()[0]) : 0;
}

bool CheckTunareGive(Database& db, Client& c, NPC& npc, const Tunare::Route& route,
    uint64_t run, const EQEmu::SharedCore::OriginalTradeRecipe::Offers& offers,
    const EQEmu::SharedCore::OriginalTradeRecipe::DebitPlan& debit, std::string& stamp)
{
    if (OfferedTunare(c, npc) != &route || c.IsSpiritShrouded()) return false;
    const auto who = c.CharacterID();
    auto engines = Q(db,
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
        "AND table_name IN ('por_tunare_orgaf','por_tunare_chalice_sources','por_tunare_exchange_receipts')");
    if (engines.RowCount() != 1 || N(engines.begin()[0]) != 3) return false;
    if (route.give == Tunare::Give::Final) {
        EQ::ItemInstance* chalice = nullptr;
        for (std::size_t i = 0; i < debit.units.size(); ++i) {
            if (debit.units[i]) chalice = c.GetInv().GetItem(offers[i].slot);
        }
        if (!chalice || chalice->GetID() != Tunare::Chalice ||
            chalice->GetCustomData(Tunare::OwnerTag) != std::to_string(who)) return false;
        const auto token = chalice->GetCustomData(Tunare::SourceTag);
        if (!Tunare::SourceKey(token) || (!stamp.empty() && stamp != token)) return false;
        auto source = Q(db, fmt::format(
            "SELECT shared_task_id FROM por_tunare_chalice_sources WHERE source_key='{}' "
            "AND character_id={} AND npc_type_id={} AND item_guid>0 FOR UPDATE",
            token, who, Tunare::Guardian));
        if (source.RowCount() != 1 || !N(source.begin()[0]) ||
            !TunareCompleted(db, who, Tunare::Raid, N(source.begin()[0]))) return false;
        for (const auto task : {920346u, 920347u, 920348u}) {
            if (!TunareCompleted(db, who, task)) return false;
        }
        if (Q(db, fmt::format(
                "SELECT character_id FROM por_tunare_exchange_receipts WHERE character_id={} "
                "OR source_key='{}' FOR UPDATE", who, token)).RowCount()) return false;
        stamp = token;
        return true;
    }

    if (!run || c.GetSharedTaskId() != static_cast<int64_t>(run) ||
        !c.IsTaskActivityActive(route.task, route.activity)) return false;
    auto member = Q(db, fmt::format(
        "SELECT t.id FROM shared_tasks t JOIN shared_task_members m ON m.shared_task_id=t.id "
        "JOIN shared_task_activity_state a ON a.shared_task_id=t.id AND a.activity_id={} "
        "JOIN shared_task_activity_state g ON g.shared_task_id=t.id AND g.activity_id={} "
        "WHERE t.id={} AND t.task_id={} AND m.character_id={} AND a.completed_time IS NOT NULL "
        "AND g.completed_time IS NULL AND g.done_count=0 "
        "AND t.completion_time IS NULL AND (t.expire_time IS NULL OR t.expire_time>NOW()) FOR UPDATE",
        route.predecessor, route.activity, run, route.task, who));
    if (member.RowCount() != 1) return false;
    if (Q(db, fmt::format(
            "SELECT request_key FROM kithicor_poison_custody WHERE recipe_kind=4 AND recipe_context=x'{}' "
            "AND shared_task_id={} AND physical=1 AND abort_only=0 FOR UPDATE",
            Hex(Tunare::Context(route)), run)).RowCount()) return false;
    if (route.give != Tunare::Give::Orgaf) return true;
    if (npc.IsEngaged() || !TunareCompleted(db, who, 920347)) return false;

    auto old = Q(db, "SELECT shared_task_id,state,dead_at FROM por_tunare_orgaf WHERE spawn_id=105177 FOR UPDATE");
    if (!old.RowCount()) return true;
    auto& row = old.begin();
    const auto old_run = N(row[0]);
    const auto state = N(row[1]);
    if (old.RowCount() != 1 || !old_run || old_run == run || state > 1) return false;
    if (Q(db, fmt::format(
            "SELECT request_key FROM kithicor_poison_custody WHERE shared_task_id={} AND recipe_kind=4 "
            "AND recipe_context=x'54554E3103' AND settled=0 FOR UPDATE", old_run)).RowCount()) return false;
    if (state == 1 && Q(db, fmt::format(
            "SELECT id FROM completed_shared_tasks WHERE id={} AND task_id=920348 "
            "AND completion_time IS NOT NULL FOR UPDATE", old_run)).RowCount() == 1) return true;
    if (!npc.GetEntityVariable(Tunare::OrgafTag).empty() || Q(db, fmt::format(
            "SELECT id FROM shared_tasks WHERE id={} AND task_id=920348 "
            "AND (expire_time IS NULL OR expire_time>NOW()) FOR UPDATE", old_run)).RowCount()) return false;
    if (state == 0) return !row[2];

    // With no archived key collection, only reclaim a genuinely terminal old
    // lease after all native live or restorable key custody is gone.
    const auto dead_at = N(row[2]);
    if (!dead_at || dead_at > static_cast<uint64_t>(std::time(nullptr))) return false;
    for (const auto& entry : entity_list.GetCorpseList()) {
        auto* corpse = entry.second;
        if (!corpse || !corpse->IsNPCCorpse() || corpse->GetNPCTypeID() != route.npc) continue;
        if (std::ranges::any_of(corpse->GetLootItems(),
                [](const auto* loot) { return loot && loot->item_id == 85079; })) return false;
    }
    return Q(db,
        "SELECT id FROM zone_state_spawns WHERE zone_id=378 AND instance_id=0 "
        "AND is_corpse=1 AND npc_id=378168 AND decay_in_seconds>0 "
        "AND (created_at IS NULL OR UNIX_TIMESTAMP(created_at)+decay_in_seconds>UNIX_TIMESTAMP()) "
        "LIMIT 1 FOR UPDATE").RowCount() == 0;
}

void CommitTunareGive(Database& db, Client& client, const Tunare::Route& route, uint64_t run,
    const std::string& stamp, const std::string& key, uint32_t output_guid)
{
    if (route.give == Tunare::Give::Orgaf) {
        auto prior = Q(db, "SELECT shared_task_id FROM por_tunare_orgaf WHERE spawn_id=105177 FOR UPDATE");
        const auto result = prior.RowCount() == 1
            ? Q(db, fmt::format(
                "UPDATE por_tunare_orgaf SET shared_task_id={},character_id={},fight_at=UNIX_TIMESTAMP()+120,"
                "state=0,dead_at=NULL WHERE spawn_id=105177 AND shared_task_id={}", run, client.CharacterID(), N(prior.begin()[0])))
            : Q(db, fmt::format(
                "INSERT INTO por_tunare_orgaf(spawn_id,shared_task_id,character_id,fight_at,state) "
                "VALUES (105177,{},{},UNIX_TIMESTAMP()+120,0)", run, client.CharacterID()));
        if (result.RowsAffected() != 1) throw std::runtime_error("Tunare Orgaf lease not recorded");
    } else if (route.give == Tunare::Give::Final) {
        if (!output_guid || !Tunare::SourceKey(stamp) || Q(db, fmt::format(
                "INSERT INTO por_tunare_exchange_receipts(character_id,source_key,request_key,output_guid,created_at) "
                "VALUES ({},'{}','{}',{},UNIX_TIMESTAMP())",
                client.CharacterID(), stamp, key, output_guid)).RowsAffected() != 1)
            throw std::runtime_error("Tunare final exchange receipt not recorded");
    }
}
