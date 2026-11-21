#include <cmath>

// Native witnesses for the two final Normal Longshadow missions.
namespace DodhLongshadowTravel {
constexpr uint32 Trailing = 920254;
constexpr uint32 Traitor = 920255;
constexpr uint32 TraitorChest = 363053;
constexpr uint32 TraitorBoss = 363064;
constexpr uint32 TraitorSpawn = 260935;
constexpr uint32 Informant = 362940;
constexpr const char* FinaleRun = "rof2_dodh_longshadow_scene_run";
constexpr const char* TraitorPhase = "rof2_dodh_traitor_phase";
constexpr const char* TailEndpoint = "rof2_dodh_tail_endpoint";

bool InFinale(uint32 task)
{
    const auto* mission = Find(task);
    if (!mission || !zone || !zone->GetInstanceID() || zone->GetZoneID() != mission->zone ||
        zone->GetInstanceVersion() != mission->version) return false;
    const auto* dz = zone->GetDynamicZone();
    return dz && dz->GetType() == DynamicZoneType::Mission && dz->IsValid() && !dz->IsExpired() &&
        dz->GetInstanceID() == zone->GetInstanceID() &&
        dz->GetName() == (task == Traitor ? "Confronting a Traitor" : "Trailing Longshadow");
}

bool IsTraitorChest(NPC& npc)
{
    return InFinale(Traitor) && npc.GetNPCTypeID() == TraitorChest;
}

bool IsFinaleActor(NPC& npc)
{
    if (npc.GetSpawnPointID() || npc.GetEntityVariable(FinaleRun).empty()) return false;
    return InFinale(Trailing) || IsTraitorChest(npc);
}

bool IsTraitorCorpse(Corpse& corpse)
{
    return InFinale(Traitor) && corpse.GetNPCTypeID() == TraitorChest &&
        corpse.GetEntityVariable(CorpseSource).rfind("920255:", 0) == 0;
}

bool OwnFinale(Client& client, uint32 task)
{
    const auto* mission = ActiveMission(client);
    if (!mission || mission->task != task || !InFinale(task)) return false;
    const auto* owned = OwnedMission(client, *mission, false);
    return owned && owned->GetInstanceID() == zone->GetInstanceID();
}

bool AllowTraitorOpen(Client& client, NPC& npc)
{
    return IsTraitorChest(npc) && npc.GetSpawnPointID() == 0 &&
        client.IsTaskActivityActive(Traitor, 1) && OwnFinale(client, Traitor) &&
        npc.GetEntityVariable(FinaleRun) == std::to_string(client.GetSharedTaskId());
}

bool IsTraitorBoss(NPC& npc)
{
    return InFinale(Traitor) && npc.GetNPCTypeID() == TraitorBoss && npc.GetSpawnPointID() == TraitorSpawn;
}

bool ConfrontTraitor(Client& client, NPC& npc)
{
    if (!IsTraitorBoss(npc) || !OwnFinale(client, Traitor)) return false;
    if (client.IsTaskActivityActive(Traitor, 2)) {
        if (!Record(client, *Find(Traitor), 2)) return true;
        npc.Say("You should never have followed me.");
    } else if (!client.IsTaskActivityActive(Traitor, 3)) {
        return false;
    }
    if (!npc.GetEntityVariable(FinaleRun).empty()) return true;
    npc.SetEntityVariable(FinaleRun, std::to_string(client.GetSharedTaskId()));
    npc.SetEntityVariable(TraitorPhase, "0");
    AppearanceStruct appearance{};
    appearance.race_id = Race::Drachnid2;
    npc.SendIllusionPacket(appearance);
    npc.AddToHateList(&client, 1, 0, false);
    return true;
}

int64_t CapFinaleDamage(NPC& npc, Mob* attacker, int64_t damage)
{
    if (damage <= 0 || !IsTraitorBoss(npc)) return damage;
    if (npc.GetEntityVariable(FinaleRun).empty()) {
        npc.WipeHateList();
        return 0;
    }
    auto* client = DodhLongshadowScene::ResponsiblePlayer(attacker);
    if (!client || !client->IsTaskActivityActive(Traitor, 3) ||
        npc.GetEntityVariable(FinaleRun) != std::to_string(client->GetSharedTaskId()) ||
        !zone->GetDynamicZone()->HasMember(client->CharacterID())) return 0;

    const auto phase_text = npc.GetEntityVariable(TraitorPhase);
    if (phase_text == "3") return damage;
    const auto phase = Strings::ToUnsignedInt(phase_text, UINT32_MAX);
    constexpr std::array percentages{70, 50, 30};
    if (phase >= percentages.size()) return 0;
    const auto threshold = (npc.GetMaxHP() * percentages[phase] + 99) / 100;
    const auto allowed = std::max<int64_t>(0, npc.GetHP() - threshold);
    if (damage < allowed) return damage;

    auto room = database.QueryDatabase(fmt::format(
        "SELECT r.boss_x,r.boss_y,r.boss_z,r.boss_heading FROM dodh_longshadow_traitor_chests c "
        "JOIN dodh_longshadow_traitor_rooms r ON r.chamber=1+MOD(c.first_boss_chamber-1+{},3) "
        "WHERE c.shared_task_id={} AND c.accepted_time={} AND c.instance_id={} AND c.opened_by IS NOT NULL",
        phase, client->GetSharedTaskId(), client->GetTaskState()->GetActiveSharedTask().accepted_time,
        zone->GetInstanceID()), false);
    if (!room.Success() || room.RowCount() != 1) return allowed;
    auto row = room.begin();
    char* location[] = {row[0], row[1], row[2], row[3]};
    const auto point = DodhLongshadowScene::Point(location);
    if (!point) return allowed;
    npc.SetEntityVariable(TraitorPhase, std::to_string(phase + 1));
    npc.SpellFinished(3791, &npc);
    npc.WipeHateList();
    npc.GMMove(*point, true);
    return allowed;
}

bool PrepareFinaleDeath(NPC& npc, Mob* attacker)
{
    const bool chest = IsTraitorChest(npc);
    const bool boss = IsTraitorBoss(npc);
    const bool informant = InFinale(Trailing) && npc.GetNPCTypeID() == Informant &&
        npc.GetSpawnPointID() == 0;
    if (!chest && !boss && !informant) return true;

    const auto task = informant ? Trailing : Traitor;
    auto* client = DodhLongshadowScene::ResponsiblePlayer(attacker);
    if (!client || !OwnFinale(*client, task) || npc.GetDepop()) return false;
    const auto run = client->GetSharedTaskId();
    if (npc.GetEntityVariable(FinaleRun) != std::to_string(run)) return false;
    if (informant) return Record(*client, *Find(task), 2);
    if (boss) {
        return npc.GetEntityVariable(TraitorPhase) == "3" &&
            Record(*client, *Find(task), 3);
    }
    if (!AllowTraitorOpen(*client, npc) ||
        !DodhCocoonOpen::ScopedOpen::Matches(&npc, attacker) ||
        !database.GetItem(54608) || !npc.GetLootItems().empty()) return false;
    const auto opened = database.QueryDatabase(fmt::format(
        "UPDATE dodh_longshadow_traitor_chests c JOIN shared_tasks live ON live.id=c.shared_task_id "
        "AND live.task_id=920255 AND UNIX_TIMESTAMP(live.accepted_time)=c.accepted_time "
        "JOIN shared_task_activity_state a ON a.shared_task_id=live.id AND a.activity_id=1 "
        "SET c.opened_by={} WHERE c.shared_task_id={} AND c.instance_id={} AND c.opened_by IS NULL "
        "AND a.done_count=0 AND a.completed_time IS NULL AND live.completion_time IS NULL "
        "AND (live.expire_time IS NULL OR live.expire_time>NOW())",
        client->CharacterID(), run, zone->GetInstanceID()), false);
    if (!opened.Success() || opened.RowsAffected() != 1) return false;
    npc.AddItem(uint32_t(54608), 1, false);
    return true;
}

void ReplayTraitorChest(Client& client, const Mission& mission)
{
    if (!OwnFinale(client, Traitor)) return;
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() == TraitorChest && npc->GetSpawnPointID() == 260900 && !npc->GetDepop())
            npc->Depop(false);
    }
    if (!client.IsTaskActivityActive(Traitor, 1)) return;
    const auto run = client.GetSharedTaskId();
    const auto instance = zone->GetInstanceID();
    const auto stamp = std::to_string(run);
    const auto corpse_stamp = fmt::format("920255:{}", run);
    for (const auto& entry : entity_list.GetCorpseList()) {
        auto* corpse = entry.second;
        if (corpse->GetNPCTypeID() == TraitorChest &&
            corpse->GetEntityVariable(CorpseSource) == corpse_stamp) return;
    }
    const auto picked = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_traitor_chests "
        "(shared_task_id,accepted_time,instance_id,chamber,x,y,z,heading,first_boss_chamber) "
        "SELECT live.id,UNIX_TIMESTAMP(live.accepted_time),i.id,r.chamber,r.x,r.y,r.z,r.heading,1+FLOOR(RAND()*3) "
        "FROM shared_tasks live{} CROSS JOIN dodh_longshadow_traitor_rooms r "
        "WHERE live.id={} AND live.task_id=920255 AND m.character_id={} AND i.id={} AND i.zone={} AND i.version={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND (SELECT COUNT(*) FROM dodh_longshadow_traitor_rooms)=3 ORDER BY RAND() LIMIT 1 "
        "ON DUPLICATE KEY UPDATE shared_task_id=dodh_longshadow_traitor_chests.shared_task_id",
        ScopeJoin, run, client.CharacterID(), instance, mission.zone, mission.version), false);
    if (!picked.Success()) return;
    auto due = database.QueryDatabase(fmt::format(
        "SELECT c.x,c.y,c.z,c.heading,c.opened_by FROM dodh_longshadow_traitor_chests c "
        "JOIN shared_tasks live ON live.id=c.shared_task_id AND live.task_id=920255 "
        "AND UNIX_TIMESTAMP(live.accepted_time)=c.accepted_time "
        "WHERE c.shared_task_id={} AND c.instance_id={} AND NOT EXISTS ("
        "SELECT 1 FROM dodh_longshadow_item_sources s WHERE s.shared_task_id=c.shared_task_id "
        "AND s.task_id=920255 AND s.accepted_time=c.accepted_time AND s.instance_id=c.instance_id "
        "AND s.item_id=54608 AND s.source_kind='traitor_chest')",
        run, instance), false);
    if (!due.Success() || due.RowCount() != 1) return;
    auto row = due.begin();
    char* position[] = {row[0], row[1], row[2], row[3]};
    const auto point = DodhLongshadowScene::Point(position);
    const auto* type = content_db.LoadNPCTypesData(TraitorChest);
    if (!point || !type || !database.GetItem(54608)) return;
    const bool opened = row[4] != nullptr;
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() != TraitorChest || npc->GetDepop()) continue;
        if (!opened && npc->GetSpawnPointID() == 0 && npc->GetEntityVariable(FinaleRun) == stamp) return;
        npc->Depop(false);
    }
    if (!opened) {
        auto* chest = new NPC(type, nullptr, *point, GravityBehavior::Water);
        chest->SetEntityVariable(FinaleRun, stamp);
        entity_list.AddNPC(chest);
        return;
    }
    NPC appearance(type, nullptr, *point, GravityBehavior::Water, true);
    LootItems loot;
    auto* drop = new LootItem{};
    drop->item_id = 54608;
    drop->charges = 1;
    loot.push_back(drop);
    auto* corpse = new Corpse(&appearance, &loot, TraitorChest, &type, RuleI(NPC, MajorNPCCorpseDecayTime));
    corpse->SetEntityVariable(CorpseSource, corpse_stamp);
    const auto* dz = zone->GetDynamicZone();
    for (const auto& entry : entity_list.GetClientList()) {
        if (entry.second->GetSharedTaskId() == run && dz && dz->HasMember(entry.second->CharacterID()))
            corpse->AddLooter(entry.second);
    }
    entity_list.AddCorpse(corpse);
    corpse->Spawn();
}

bool TakeWhiteclawTravel(Client& client, NPC& npc)
{
    const auto* mission = ActiveMission(client);
    if (!mission || mission->task != Trailing || !zone || zone->GetZoneID() != 358 ||
        zone->GetInstanceVersion() != 0 || zone->GetInstanceID() ||
        npc.GetNPCTypeID() != 358120 || npc.GetSpawnPointID() != 102230) return false;
    const auto* owned = OwnedMission(client, *mission, false);
    if (!owned || !owned->HasZoneInLocation()) return true;
    if (client.IsTaskActivityActive(Trailing, 0) && !Record(client, *mission, 0)) return true;
    const auto& point = owned->GetZoneInLocation();
    client.MovePC(mission->zone, owned->GetInstanceID(), point.x, point.y, point.z, point.heading);
    return true;
}
}

namespace DodhLongshadowTravel {
void ReplayTrailing(Client& client)
{
    if (!client.IsTaskActivityActive(Trailing, 1) || !OwnFinale(client, Trailing)) return;
    const auto stamp = std::to_string(client.GetSharedTaskId());
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* elder = entry.second;
        if (elder->GetNPCTypeID() != 362313 || elder->GetSpawnPointID() != 263795 ||
            elder->GetDepop() || elder->GetHP() <= 0 ||
            elder->GetEntityVariable(TailEndpoint) != stamp) continue;
        const auto& point = elder->GetPosition();
        if (std::abs(point.x - 885.37f) <= 12.0f && std::abs(point.y - 601.23f) <= 12.0f &&
            std::abs(point.z + 42.066f) <= 12.0f) Record(client, *Find(Trailing), 1);
        return;
    }
}
}
