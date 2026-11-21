namespace DodhHardNotebookChest {
constexpr uint32 Task = 920229;
constexpr uint32 Actor = 354994;
constexpr const char* Marker = "rof2_dodh_hard_notebook_chest";

inline const DynamicZone* Instance()
{
    if (!zone || zone->GetZoneID() != 354 || zone->GetInstanceVersion() != 20 ||
        !zone->GetInstanceID()) return nullptr;
    const auto* dz = zone->GetDynamicZone();
    return dz && dz->GetType() == DynamicZoneType::Mission && dz->IsValid() && !dz->IsExpired() &&
        dz->GetInstanceID() == zone->GetInstanceID() && dz->GetName() == "The Lost Notebook (Hard)" ? dz : nullptr;
}

inline uint64_t MemberRun(Client& client)
{
    const auto* dz = Instance();
    return dz && client.GetSharedTaskId() > 0 && dz->HasMember(client.CharacterID())
        ? uint64_t(client.GetSharedTaskId()) : 0;
}

constexpr const char* ArchiveJoin =
    " JOIN completed_shared_tasks done ON done.id=live.id AND done.task_id=920229 "
    " AND done.accepted_time=live.accepted_time AND done.completion_time IS NOT NULL "
    " JOIN completed_shared_task_members m ON m.shared_task_id=done.id "
    " JOIN shared_task_dynamic_zones l ON l.shared_task_id=done.id "
    " JOIN dynamic_zones d ON d.id=l.dynamic_zone_id AND d.type=4 "
    " JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=d.id AND dm.character_id=m.character_id "
    " JOIN instance_list i ON i.id=d.instance_id AND i.zone=354 AND i.version=20 ";
constexpr const char* ActiveArchive =
    " AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
    " AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP() ";

bool IsActor(NPC& npc)
{
    return npc.GetNPCTypeID() == Actor && Instance();
}

bool IsCorpse(Corpse& corpse)
{
    return corpse.GetNPCTypeID() == Actor && Instance();
}

bool AllowOpen(Client& client, NPC& npc)
{
    const auto run = MemberRun(client);
    return run && IsActor(npc) && npc.GetSpawnPointID() == 0 &&
        npc.GetEntityVariable(Marker) == std::to_string(run);
}

bool PrepareDeath(NPC& npc, Mob* attacker)
{
    if (!IsActor(npc)) return true;
    if (!attacker || !attacker->IsClient() || npc.GetDepop() || !npc.GetLootItems().empty() ||
        !DodhCocoonOpen::ScopedOpen::Matches(&npc, attacker)) return false;
    auto& client = *attacker->CastToClient();
    if (!AllowOpen(client, npc)) return false;
    const auto run = MemberRun(client);
    const auto instance = zone->GetInstanceID();
    auto item = database.QueryDatabase(fmt::format(
        "SELECT c.item_id FROM dodh_hard_notebook_chests c WHERE c.shared_task_id={} "
        "AND c.instance_id={} AND c.opened_by IS NULL AND c.loot_guid IS NULL", run, instance), false);
    if (!item.Success() || item.RowCount() != 1) return false;
    const auto item_id = Strings::ToUnsignedInt(item.begin()[0]);
    if (!item_id || !database.GetItem(item_id)) return false;
    const auto open = database.QueryDatabase(fmt::format(
        "UPDATE dodh_hard_notebook_chests c JOIN shared_tasks live ON live.id=c.shared_task_id "
        "AND live.task_id=920229{} SET c.opened_by={} WHERE live.id={} AND c.instance_id={} "
        "AND d.id={} AND i.id=c.instance_id AND m.character_id={} AND c.item_id={} "
        "AND c.opened_by IS NULL AND c.loot_guid IS NULL{}",
        ArchiveJoin, client.CharacterID(), run, instance, Instance()->GetID(), client.CharacterID(),
        item_id, ActiveArchive), false);
    if (!open.Success() || open.RowsAffected() != 1) return false;
    npc.AddItem(item_id, 1, false);
    return true;
}

void Replay(Client& client)
{
    const auto run = MemberRun(client);
    if (!run || !client.IsTaskCompleted(Task)) return;
    const auto* dz = Instance();
    const auto instance = dz->GetInstanceID();
    const auto stamp = std::to_string(run);
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() == Actor && !npc->GetDepop() && npc->GetEntityVariable(Marker) == stamp) return;
    }
    for (const auto& entry : entity_list.GetCorpseList()) {
        if (entry.second->GetNPCTypeID() == Actor && entry.second->GetEntityVariable(Marker) == stamp) return;
    }
    const auto created = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_hard_notebook_chests (shared_task_id,instance_id,item_id) "
        "SELECT live.id,i.id,drops.item_id FROM shared_tasks live{} "
        "CROSS JOIN lootdrop_entries drops JOIN loottable_entries pool "
        "ON pool.lootdrop_id=drops.lootdrop_id AND pool.loottable_id=304070105 "
        "WHERE live.id={} AND live.task_id=920229 AND m.character_id={} AND d.id={} AND i.id={} "
        "AND drops.lootdrop_id=304070106 AND (SELECT COUNT(*) FROM completed_shared_task_activity_state a "
        "WHERE a.shared_task_id=live.id AND a.activity_id BETWEEN 0 AND 9 AND a.done_count=1 "
        "AND a.completed_time IS NOT NULL)=10{} ORDER BY RAND() LIMIT 1 "
        "ON DUPLICATE KEY UPDATE shared_task_id=dodh_hard_notebook_chests.shared_task_id",
        ArchiveJoin, run, client.CharacterID(), dz->GetID(), instance, ActiveArchive), false);
    if (!created.Success()) return;
    auto pending = database.QueryDatabase(fmt::format(
        "SELECT c.item_id,c.opened_by FROM dodh_hard_notebook_chests c "
        "JOIN shared_tasks live ON live.id=c.shared_task_id AND live.task_id=920229{} "
        "WHERE live.id={} AND m.character_id={} AND d.id={} AND i.id=c.instance_id "
        "AND c.instance_id={} AND c.loot_guid IS NULL{}",
        ArchiveJoin, run, client.CharacterID(), dz->GetID(), instance, ActiveArchive), false);
    if (!pending.Success() || pending.RowCount() != 1 || !dz->HasZoneInLocation()) return;
    const auto item_id = Strings::ToUnsignedInt(pending.begin()[0]);
    const auto* type = content_db.LoadNPCTypesData(Actor);
    if (!item_id || !database.GetItem(item_id) || !type) return;
    const auto& entry = dz->GetZoneInLocation();
    const glm::vec4 point{entry.x, entry.y, entry.z, entry.heading};
    if (!pending.begin()[1]) {
        auto* npc = new NPC(type, nullptr, point, GravityBehavior::Water);
        npc->SetEntityVariable(Marker, stamp);
        entity_list.AddNPC(npc);
        return;
    }
    NPC appearance(type, nullptr, point, GravityBehavior::Water, true);
    LootItems loot;
    auto* drop = new LootItem{};
    drop->item_id = item_id;
    drop->charges = 1;
    loot.push_back(drop);
    auto* corpse = new Corpse(&appearance, &loot, Actor, &type, RuleI(NPC, MajorNPCCorpseDecayTime));
    corpse->SetEntityVariable(Marker, stamp);
    for (const auto& member : entity_list.GetClientList()) {
        if (MemberRun(*member.second) == run) corpse->AddLooter(member.second);
    }
    entity_list.AddCorpse(corpse);
    corpse->Spawn();
}

inline std::string LootScope(Client& client, uint64_t run, uint32_t item)
{
    const auto* dz = Instance();
    if (!dz || !run || MemberRun(client) != run) return {};
    return fmt::format(
        "WHERE live.id={} AND live.task_id=920229 AND c.instance_id={} AND i.id=c.instance_id AND d.id={} "
        "AND m.character_id={} AND c.item_id={} AND c.opened_by IS NOT NULL{}",
        run, dz->GetInstanceID(), dz->GetID(), client.CharacterID(), item, ActiveArchive);
}

constexpr const char* LootJoin =
    " FROM dodh_hard_notebook_chests c JOIN shared_tasks live ON live.id=c.shared_task_id ";
} // namespace DodhHardNotebookChest
