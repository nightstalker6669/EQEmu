// Included by the original trade translation unit for the native hail witness.
#include "common/dodh_longshadow_travel.h"
#include "doors.h"

namespace DodhLongshadowTravel {
const Mission* ActiveMission(Client& client)
{
    const auto* state = client.GetTaskState();
    return state && client.GetSharedTaskId() > 0 ? Find(state->GetActiveSharedTask().task_id) : nullptr;
}

const DynamicZone* OwnedMission(Client& client, const Mission& mission, bool may_be_complete)
{
    const auto& active = client.GetTaskState()->GetActiveSharedTask();
    if (active.accepted_time <= 0) return nullptr;
    auto row = database.QueryDatabase(fmt::format(
        "SELECT dz.id,i.id FROM shared_tasks live{}"
        "WHERE live.id={} AND live.task_id={} AND m.character_id={} AND i.zone={} AND i.version={} "
        "AND UNIX_TIMESTAMP(live.accepted_time)={} AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP(){}",
        ScopeJoin, client.GetSharedTaskId(), mission.task, client.CharacterID(), mission.zone, mission.version,
        active.accepted_time, may_be_complete ? "" : " AND live.completion_time IS NULL"), false);
    if (!row.Success() || row.RowCount() != 1) return nullptr;
    const auto id = SharedTaskAdmission::Number(row.begin()[0]);
    const auto instance = SharedTaskAdmission::Number(row.begin()[1]);
    if (!id || !*id || !instance || !*instance) return nullptr;
    for (const auto* dz : client.GetDynamicZones(mission.zone, mission.version)) {
        if (dz && dz->GetID() == *id && dz->GetInstanceID() == *instance &&
            dz->GetType() == DynamicZoneType::Mission && dz->IsValid() && !dz->IsExpired() &&
            dz->HasMember(client.CharacterID())) return dz;
    }
    return nullptr;
}

bool Record(Client&, const Mission&, uint32_t);

bool UseDoor(Client& client, Doors& door)
{
    // Notebook uses the same public gate, but its own v20 arrival
    // objective is credited by the scoped Notebook zone hook.
    const auto* state = client.GetTaskState();
    if (state && client.GetSharedTaskId() > 0 && state->GetActiveSharedTask().task_id == 920229 &&
        zone && zone->GetZoneID() == 358 && zone->GetInstanceVersion() == 0 && !zone->GetInstanceID() &&
        door.GetDoorDBID() == 16287 && door.GetDoorID() == 1) {
        constexpr Mission notebook{920229, 354, 20, 0, 358, 16287, 1, 0, 0};
        const auto* owned = OwnedMission(client, notebook, false);
        if (owned && owned->HasZoneInLocation()) {
            const auto& point = owned->GetZoneInLocation();
            client.MovePC(notebook.zone, owned->GetInstanceID(), point.x, point.y, point.z, point.heading);
        }
        return true;
    }
    const auto* mission = ActiveMission(client);
    if (!mission || !zone) return false;
    const auto instance = zone->GetInstanceID();
    const auto version = zone->GetInstanceVersion();
    const auto zone_id = zone->GetZoneID();
    const bool entering = mission->entrance_door && !instance && version == 0 && zone_id == mission->outside_zone &&
        door.GetDoorDBID() == mission->entrance_door && door.GetDoorID() == mission->entrance_switch;
    const bool returning = mission->exit_door && instance && zone_id == mission->zone && version == mission->version &&
        door.GetDoorDBID() == mission->exit_door && door.GetDoorID() == mission->exit_switch;
    if (!entering && !returning) return false;
    const bool bellfast = mission->task >= 920270 && mission->task <= 920273;
    if (bellfast) {
        const float dz = client.GetZ() - door.GetZ();
        if (DistanceSquaredNoZ(client.GetPosition(), door.GetPosition()) > 2025.0f || dz * dz > 900.0f)
            return true;
    }
    const auto* owned = OwnedMission(client, *mission, returning);
    if (!owned) return bellfast;
    if (entering) {
        if (BellfastApproach(mission->task, 0) && client.IsTaskActivityActive(mission->task, 0) &&
            !Record(client, *mission, 0)) return true;
        if (mission->task == 920273 && (!DodhBellfastNeran::CanEnter(client) ||
            client.IsTaskActivityActive(mission->task, 0) || client.IsTaskActivityActive(mission->task, 1)))
            return true;
        if (!owned->HasZoneInLocation()) return false;
        if (mission->task == 920253 && client.IsTaskActivityActive(mission->task, 0)) {
            client.Message(Chat::Yellow, "Speak with Spiritualist Ryianna before entering the dens.");
            return true;
        }
        const auto& point = owned->GetZoneInLocation();
        client.MovePC(mission->zone, owned->GetInstanceID(), point.x, point.y, point.z, point.heading);
    } else {
        if (owned->GetInstanceID() != instance) return false;
        const auto& point = owned->GetSafeReturnLocation();
        if (point.zone_id != mission->outside_zone) return false;
        client.MovePC(point.zone_id, 0, point.x, point.y, point.z, point.heading);
    }
    return true;
}

bool Record(Client& client, const Mission& mission, uint32_t activity)
{
    if (!client.IsTaskActivityActive(mission.task, activity)) return false;
    const auto* owned = OwnedMission(client, mission, false);
    if (!owned) return false;
    const bool bellfast_public = BellfastApproach(mission.task, activity) ||
        NeranPublicActivity(mission.task, activity);
    const bool public_source = (mission.task == 920254 && activity == 0) ||
        IsReturn(mission.task, activity) || bellfast_public;
    if (bellfast_public &&
        (!zone || zone->GetInstanceID() || zone->GetInstanceVersion() != 0 ||
         zone->GetZoneID() != mission.outside_zone)) return false;
    if (!public_source && (!zone || zone->GetInstanceID() != owned->GetInstanceID())) return false;
    auto wrote = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_travel_events "
        "(shared_task_id,task_id,activity_id,character_id,accepted_time,instance_id) "
        "SELECT live.id,live.task_id,{},m.character_id,UNIX_TIMESTAMP(live.accepted_time),dz.instance_id "
        "FROM shared_tasks live{}"
        "JOIN shared_task_activity_state a ON a.shared_task_id=live.id AND a.activity_id={} "
        "AND a.done_count=0 AND a.completed_time IS NULL "
        "WHERE live.id={} AND live.task_id={} AND m.character_id={} AND i.zone={} AND i.version={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP() AND i.id={} AND dz.id={} "
        "ON DUPLICATE KEY UPDATE character_id=dodh_longshadow_travel_events.character_id",
        activity, ScopeJoin, activity, client.GetSharedTaskId(), mission.task, client.CharacterID(),
        mission.zone, mission.version, owned->GetInstanceID(), owned->GetID()), false);
    if (!wrote.Success()) return false;
    if (wrote.RowsAffected() == 1) return true;
    const auto found = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM dodh_longshadow_travel_events WHERE shared_task_id={} AND task_id={} "
        "AND activity_id={} AND character_id={} AND accepted_time={} AND instance_id={}",
        client.GetSharedTaskId(), mission.task, activity, client.CharacterID(),
        client.GetTaskState()->GetActiveSharedTask().accepted_time, owned->GetInstanceID()), false);
    return found.Success() && found.RowCount() == 1;
}

void ReplayOrb(Client&, const Mission&);
void ReplayTraitorChest(Client&, const Mission&);
void ReplayTrailing(Client&);

constexpr const char* CavernsScene = "rof2_dodh_bellfast_caverns_scene";

bool CavernsInstance()
{
    return zone && zone->GetZoneID() == 360 && zone->GetInstanceVersion() == 1 && zone->GetInstanceID();
}

bool AllowCavernsDeath(NPC& npc, Mob* killer)
{
    if (!CavernsInstance() || npc.GetNPCTypeID() != 360056 || npc.GetSpawnPointID() != 259600)
        return true;
    auto* owner = killer ? killer->GetUltimateOwner() : nullptr;
    if (!owner || !owner->IsClient()) return false;
    auto& client = *owner->CastToClient();
    const auto* mission = ActiveMission(client);
    if (!mission || mission->task != 920271 || !client.IsTaskActivityActive(920271, 3))
        return false;
    const auto* owned = OwnedMission(client, *mission, false);
    return owned && owned->GetInstanceID() == zone->GetInstanceID() &&
        npc.GetEntityVariable(CavernsScene) == std::to_string(client.GetSharedTaskId());
}

bool PrepareBellfastLoot(NPC& npc, Mob* killer)
{
    namespace Custody = EQEmu::SharedCore::DodhLongshadowCustody;
    if (!zone || !zone->GetInstanceID()) return true;
    for (const auto& route : Custody::Sources) {
        if ((route.source != Custody::Source::AlphaLarvaMeat &&
             route.source != Custody::Source::ClicktokGem &&
             route.source != Custody::Source::TherimelGem) ||
            npc.GetNPCTypeID() != route.npc || npc.GetSpawnPointID() != route.spawn ||
            zone->GetZoneID() != route.zone || zone->GetInstanceVersion() != route.version) continue;
        auto* owner = killer ? killer->GetUltimateOwner() : nullptr;
        if (!owner || !owner->IsClient()) return false;
        auto& client = *owner->CastToClient();
        const auto* mission = ActiveMission(client);
        if (!mission || mission->task != route.task || !client.IsTaskActivityActive(route.task, route.activity))
            return false;
        const auto* owned = OwnedMission(client, *mission, false);
        if (!owned || owned->GetInstanceID() != zone->GetInstanceID()) return false;

        auto existing = database.QueryDatabase(fmt::format(
            "SELECT 1 FROM dodh_longshadow_item_sources WHERE shared_task_id={} "
            "AND source_kind='{}' AND accepted_time={} AND instance_id={} AND task_id={} AND item_id={}",
            client.GetSharedTaskId(), Custody::SourceKind(route.source),
            client.GetTaskState()->GetActiveSharedTask().accepted_time,
            owned->GetInstanceID(), route.task, route.item), false);
        if (!existing.Success()) return false;
        if (existing.RowCount()) return true;

        const auto& loot = npc.GetLootItems();
        if (std::any_of(loot.begin(), loot.end(),
                [&](const auto* entry) { return entry && entry->item_id == route.item; })) return true;
        if (!database.GetItem(route.item)) return false;
        npc.AddItem(route.item, 1, false);
        return true;
    }
    return true;
}

void EnterCavernsQuarters(Client& client, const Mission& mission)
{
    const bool searching = client.IsTaskActivityActive(mission.task, 2);
    if (!searching && !client.IsTaskActivityActive(mission.task, 3)) return;
    const auto* owned = OwnedMission(client, mission, false);
    if (!owned || owned->GetInstanceID() != zone->GetInstanceID()) return;

    NPC* captain = nullptr;
    NPC* scholar = nullptr;
    std::array<NPC*, 2> guards{};
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetDepop() || npc->GetHP() <= 0) continue;
        const auto spawn = npc->GetSpawnPointID();
        if (npc->GetNPCTypeID() == 360056 && spawn == 259600) captain = npc;
        if (npc->GetNPCTypeID() == 360055 && spawn == 259599) scholar = npc;
        if (npc->GetNPCTypeID() == 360048 && spawn == 259537) guards[0] = npc;
        if (npc->GetNPCTypeID() == 360048 && spawn == 259538) guards[1] = npc;
    }
    if (!captain || !scholar || !guards[0] || !guards[1] ||
        !captain->GetEntityVariable(CavernsScene).empty()) return;
    if (searching) {
        if (!Record(client, mission, 2)) return;
    } else {
        const auto saved = database.QueryDatabase(fmt::format(
            "SELECT 1 FROM dodh_longshadow_travel_events WHERE shared_task_id={} AND task_id=920271 "
            "AND activity_id=2 AND accepted_time={} AND instance_id={}",
            client.GetSharedTaskId(), client.GetTaskState()->GetActiveSharedTask().accepted_time,
            owned->GetInstanceID()), false);
        if (!saved.Success() || saved.RowCount() != 1) return;
    }
    if (!scholar->Death(captain, 0, SPELL_UNKNOWN, EQ::skills::SkillHandtoHand)) return;
    captain->SetEntityVariable(CavernsScene, std::to_string(client.GetSharedTaskId()));
    captain->AddToHateList(&client, 1, 0, false);
    for (auto* guard : guards) guard->AddToHateList(&client, 1, 0, false);
}

void RecordArrival(Client& client)
{
    DodhHardNotebookChest::Replay(client);
    DodhBellfastHive::Replay(client);
    DodhBellfastKorlach::Replay(client);
    DodhBellfastNeran::Progress(client);
    const auto* mission = ActiveMission(client);
    if (!mission || !zone || zone->GetZoneID() != mission->zone || zone->GetInstanceVersion() != mission->version ||
        !zone->GetInstanceID()) return;
    const auto* dz = zone->GetDynamicZone();
    if (!dz || dz->GetType() != DynamicZoneType::Mission || !dz->IsValid() || dz->IsExpired() ||
        dz->GetInstanceID() != zone->GetInstanceID() || !dz->HasMember(client.CharacterID())) return;
    if (mission->task != 920254 && client.IsTaskActivityActive(mission->task, mission->arrival))
        Record(client, *mission, mission->arrival);
    if ((mission->task == 920271 && (client.IsTaskActivityActive(920271, 2) ||
                                   client.IsTaskActivityActive(920271, 3))) ||
        (mission->task == 920272 && client.IsTaskActivityActive(920272, 2))) {
        // Server XYZ from the documented /loc landmarks. Tolerances account
        // for the character origin and are emulator settings.
        const auto point = mission->task == 920271
            ? glm::vec4{1275.0f, 182.0f, -23.0f, 0.0f}
            : glm::vec4{523.0f, 808.0f, -75.0f, 0.0f};
        const float z_delta = client.GetZ() - point.z;
        if (z_delta * z_delta <= 144.0f && DistanceSquaredNoZ(client.GetPosition(), point) <= 900.0f) {
            if (mission->task == 920271) EnterCavernsQuarters(client, *mission);
            else Record(client, *mission, 2);
        }
    }
    if (mission->task == 920255)
        ReplayTraitorChest(client, *mission);
    else if (mission->task == 920254)
        ReplayTrailing(client);
    else
        ReplayOrb(client, *mission);
}
}

namespace DodhLongshadowTravel {
constexpr const char* CorpseSource = "rof2_longshadow_corpse_source";
constexpr const char* OrbTerminal = "rof2_poisonmaster_terminal";

bool IsOrbCorpse(Corpse& corpse)
{
    return zone && zone->GetZoneID() == 354 && zone->GetInstanceVersion() == 5 &&
        zone->GetInstanceID() && corpse.GetNPCTypeID() == 354311 &&
        corpse.GetEntityVariable(CorpseSource).rfind("920252:", 0) == 0;
}

bool IsPoisonmaster(NPC& npc)
{
    return zone && zone->GetZoneID() == 354 && zone->GetInstanceVersion() == 5 &&
        zone->GetInstanceID() && npc.GetNPCTypeID() == 354311 && npc.GetSpawnPointID() == 262895;
}

bool PrepareOrbDeath(NPC& npc, Mob* attacker)
{
    if (!IsPoisonmaster(npc)) return true;
    auto* client = DodhLongshadowScene::ResponsiblePlayer(attacker);
    const auto* mission = client ? ActiveMission(*client) : nullptr;
    if (!mission || mission->task != 920252) {
        // Preserve a retained actor outside a running owned Orb mission.
        const auto match = database.QueryDatabase(fmt::format(
            "SELECT 1 FROM shared_tasks live JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=live.id "
            "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 "
            "WHERE live.task_id=920252 AND dz.instance_id={} AND live.completion_time IS NULL "
            "AND (live.expire_time IS NULL OR live.expire_time>NOW())", zone->GetInstanceID()), false);
        return match.Success() && match.RowCount() == 0;
    }
    if (!client->IsTaskActivityActive(920252, 1) || !database.GetItem(54608)) return false;
    const auto run = client->GetSharedTaskId();
    if (npc.GetEntityVariable(OrbTerminal) != std::to_string(run)) return false;
    const auto* owned = OwnedMission(*client, *mission, false);
    if (!owned || owned->GetInstanceID() != zone->GetInstanceID()) return false;

    const auto& p = npc.GetPosition();
    const auto write = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_orb_deaths "
        "(shared_task_id,actor_type,spawn_id,character_id,accepted_time,instance_id,x,y,z,heading) "
        "SELECT live.id,354311,262895,m.character_id,UNIX_TIMESTAMP(live.accepted_time),i.id,{},{},{},{} "
        "FROM shared_tasks live{} JOIN shared_task_activity_state a ON a.shared_task_id=live.id AND a.activity_id=1 "
        "WHERE live.id={} AND live.task_id=920252 AND m.character_id={} AND i.id={} AND i.zone=354 "
        "AND i.version=5 AND dz.id={} AND a.done_count=0 AND a.completed_time IS NULL "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP()",
        p.x, p.y, p.z, p.w, ScopeJoin, run, client->CharacterID(), owned->GetInstanceID(), owned->GetID()), false);
    if (!write.Success() || write.RowsAffected() != 1) return false;
    npc.AddItem(uint32_t(54608), 1, false);
    return true;
}

void ReplayOrb(Client& client, const Mission& mission)
{
    if (mission.task != 920252 || (!client.IsTaskActivityActive(920252, 1) &&
        !client.IsTaskActivityActive(920252, 2))) return;
    const auto run = client.GetSharedTaskId();
    const auto stamp = fmt::format("920252:{}", run);
    for (const auto& entry : entity_list.GetCorpseList()) {
        auto* corpse = entry.second;
        if (corpse->GetNPCTypeID() == 354311 && corpse->GetEntityVariable(CorpseSource) == stamp) return;
    }
    const auto* owned = OwnedMission(client, mission, false);
    if (!owned || owned->GetInstanceID() != zone->GetInstanceID()) return;
    auto death = database.QueryDatabase(fmt::format(
        "SELECT d.x,d.y,d.z,d.heading FROM dodh_longshadow_orb_deaths d "
        "JOIN shared_tasks live ON live.id=d.shared_task_id AND live.task_id=920252 "
        "AND UNIX_TIMESTAMP(live.accepted_time)=d.accepted_time "
        "WHERE d.shared_task_id={} AND d.instance_id={} AND d.actor_type=354311 AND d.spawn_id=262895 "
        "AND NOT EXISTS (SELECT 1 FROM dodh_longshadow_item_sources s WHERE s.shared_task_id=d.shared_task_id "
        "AND s.accepted_time=d.accepted_time AND s.instance_id=d.instance_id AND s.task_id=920252 "
        "AND s.source_kind='poisonmaster_corpse' AND s.item_id=54608)",
        run, owned->GetInstanceID()), false);
    if (!death.Success() || death.RowCount() != 1) return;
    auto row = death.begin();
    char* location[] = {row[0], row[1], row[2], row[3]};
    const auto point = DodhLongshadowScene::Point(location);
    const auto* type = content_db.LoadNPCTypesData(354311);
    if (!point || !type || !database.GetItem(54608)) return;
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (IsPoisonmaster(*npc) && !npc->GetDepop()) npc->Depop(false);
    }
    NPC appearance(type, nullptr, *point, GravityBehavior::Water, true);
    LootItems loot;
    auto* drop = new LootItem{};
    drop->item_id = 54608;
    drop->charges = 1;
    loot.push_back(drop);
    auto* corpse = new Corpse(&appearance, &loot, 354311, &type, RuleI(NPC, MajorNPCCorpseDecayTime));
    corpse->SetEntityVariable(CorpseSource, stamp);
    for (const auto& entry : entity_list.GetClientList()) {
        if (entry.second->GetSharedTaskId() == run && owned->HasMember(entry.second->CharacterID()))
            corpse->AddLooter(entry.second);
    }
    entity_list.AddCorpse(corpse);
    corpse->Spawn();
}
}

#include "dodh_longshadow_finale.inc"
