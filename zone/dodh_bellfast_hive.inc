// The required Fibblebrap gem is issued only through native chest loot.
#include "dodh_cocoon_open_receipt.h"

namespace DodhBellfastHive {
constexpr uint32_t Task = 920272;
constexpr uint32_t Foreman = 355073;
constexpr uint32_t Spawn = 261609;
constexpr uint32_t Actor = 355940;
constexpr uint32_t Gem = 87009;
constexpr const char* Marker = "rof2_dodh_bellfast_hive_run";

const DynamicZone* Instance()
{
    if (!zone || zone->GetZoneID() != 355 || zone->GetInstanceVersion() != 1 ||
        !zone->GetInstanceID()) return nullptr;
    const auto* dz = zone->GetDynamicZone();
    return dz && dz->GetType() == DynamicZoneType::Mission && dz->IsValid() && !dz->IsExpired() &&
        dz->GetInstanceID() == zone->GetInstanceID() ? dz : nullptr;
}

struct Run {
    uint64_t id;
    uint32_t accepted;
    const DynamicZone* dz;

    std::string Stamp() const { return fmt::format("{}:{}", id, accepted); }
};

std::optional<Run> Member(Client& client)
{
    const auto* dz = Instance();
    const auto* state = client.GetTaskState();
    if (!dz || !state || client.GetSharedTaskId() <= 0 || !dz->HasMember(client.CharacterID()))
        return std::nullopt;
    const auto& active = state->GetActiveSharedTask();
    if (active.task_id != Task || active.accepted_time <= 0 ||
        uint64_t(active.accepted_time) > UINT32_MAX) return std::nullopt;
    return Run{uint64_t(client.GetSharedTaskId()), uint32_t(active.accepted_time), dz};
}

std::string Scope(Client& client, const Run& run)
{
    return fmt::format(
        "live.id={} AND live.task_id=920272 AND UNIX_TIMESTAMP(live.accepted_time)={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND m.character_id={} AND dz.id={} AND i.id={} AND i.zone=355 AND i.version=1 "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP()",
        run.id, run.accepted, client.CharacterID(), run.dz->GetID(), run.dz->GetInstanceID());
}

constexpr const char* ReceiptJoin =
    " JOIN dodh_bellfast_hive_chests chest ON chest.shared_task_id=live.id "
    "AND chest.accepted_time=UNIX_TIMESTAMP(live.accepted_time) AND chest.instance_id=i.id "
    "JOIN shared_task_activity_state foreman_kill ON foreman_kill.shared_task_id=live.id "
    "AND foreman_kill.activity_id=3 AND foreman_kill.done_count=1 AND foreman_kill.completed_time IS NOT NULL ";

bool IsChest(NPC& npc)
{
    return npc.GetNPCTypeID() == Actor && Instance();
}

bool AllowOpen(Client& client, NPC& npc)
{
    const auto run = Member(client);
    return run && IsChest(npc) && npc.GetSpawnPointID() == 0 && !npc.GetDepop() &&
        client.IsTaskActivityActive(Task, 4) && npc.GetEntityVariable(Marker) == run->Stamp();
}

bool PrepareDeath(NPC& npc, Mob* killer)
{
    if (IsChest(npc)) {
        if (!killer || !killer->IsClient() || !npc.GetLootItems().empty() ||
            !DodhCocoonOpen::ScopedOpen::Matches(&npc, killer)) return false;
        auto& client = *killer->CastToClient();
        if (!AllowOpen(client, npc) || !database.GetItem(Gem)) return false;
        const auto run = Member(client);
        auto open = database.QueryDatabase(fmt::format(
            "UPDATE shared_tasks live{}{} SET chest.opened_by={} WHERE {} AND chest.opened_by IS NULL",
            DodhLongshadowTravel::ScopeJoin, ReceiptJoin, client.CharacterID(), Scope(client, *run)), false);
        if (!open.Success() || open.RowsAffected() != 1) return false;
        npc.AddItem(Gem, 1, false);
        npc.SetEntityVariable("rof2_dodh_longshadow_scene_run", std::to_string(run->id));
        return true;
    }

    if (npc.GetNPCTypeID() != Foreman || npc.GetSpawnPointID() != Spawn || !Instance()) return true;
    auto* owner = killer ? killer->GetUltimateOwner() : nullptr;
    if (!owner || !owner->IsClient()) return false;
    auto& client = *owner->CastToClient();
    const auto run = Member(client);
    if (!run || !client.IsTaskActivityActive(Task, 3)) return false;
    const auto& point = npc.GetPosition();
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z) ||
        !std::isfinite(point.w)) return false;
    auto record = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_bellfast_hive_chests "
        "(shared_task_id,accepted_time,instance_id,killed_by,x,y,z,heading) "
        "SELECT live.id,UNIX_TIMESTAMP(live.accepted_time),i.id,m.character_id,{},{},{},{} "
        "FROM shared_tasks live{} "
        "JOIN shared_task_activity_state activity ON activity.shared_task_id=live.id "
        "AND activity.activity_id=3 AND activity.done_count=0 AND activity.completed_time IS NULL "
        "WHERE {} ON DUPLICATE KEY UPDATE shared_task_id=dodh_bellfast_hive_chests.shared_task_id",
        point.x, point.y, point.z, point.w, DodhLongshadowTravel::ScopeJoin, Scope(client, *run)), false);
    if (!record.Success()) return false;
    const auto receipt = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM dodh_bellfast_hive_chests WHERE shared_task_id={} AND accepted_time={} AND instance_id={}",
        run->id, run->accepted, run->dz->GetInstanceID()), false);
    return receipt.Success() && receipt.RowCount() == 1;
}

bool LootWitness(ZoneDatabase& db, Client& client, uint32_t accepted, uint32_t instance)
{
    const auto run = Member(client);
    if (!run || run->accepted != accepted || run->dz->GetInstanceID() != instance) return false;
    const auto receipt = db.QueryDatabase(fmt::format(
        "SELECT chest.opened_by FROM shared_tasks live{}{} WHERE {} AND chest.opened_by IS NOT NULL FOR UPDATE",
        DodhLongshadowTravel::ScopeJoin, ReceiptJoin, Scope(client, *run)), false);
    return receipt.Success() && receipt.RowCount() == 1;
}

void Replay(Client& client)
{
    if (!client.IsTaskActivityActive(Task, 4)) return;
    const auto run = Member(client);
    if (!run) return;
    const auto stamp = run->Stamp();
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() == Actor && !npc->GetDepop() && npc->GetEntityVariable(Marker) == stamp)
            return;
    }
    const auto corpse_stamp = fmt::format("{}:{}", Task, run->id);
    for (const auto& entry : entity_list.GetCorpseList()) {
        if (entry.second->GetNPCTypeID() == Actor &&
            entry.second->GetEntityVariable("rof2_longshadow_corpse_source") == corpse_stamp) return;
    }
    auto receipt = database.QueryDatabase(fmt::format(
        "SELECT chest.x,chest.y,chest.z,chest.heading,chest.opened_by FROM shared_tasks live{}{} "
        "WHERE {} AND NOT EXISTS (SELECT 1 FROM dodh_longshadow_item_sources source "
        "WHERE source.shared_task_id=live.id AND source.source_kind='bellfast_zlixtil_gem')",
        DodhLongshadowTravel::ScopeJoin, ReceiptJoin, Scope(client, *run)), false);
    if (!receipt.Success() || receipt.RowCount() != 1 || !database.GetItem(Gem)) return;
    const auto* type = content_db.LoadNPCTypesData(Actor);
    if (!type) return;
    auto row = receipt.begin();
    const glm::vec4 point{std::strtof(row[0],nullptr), std::strtof(row[1],nullptr),
        std::strtof(row[2],nullptr), std::strtof(row[3],nullptr)};
    if (!row[4]) {
        auto* npc = new NPC(type, nullptr, point, GravityBehavior::Water);
        npc->SetEntityVariable(Marker, stamp);
        entity_list.AddNPC(npc);
        return;
    }
    NPC appearance(type, nullptr, point, GravityBehavior::Water, true);
    LootItems loot;
    auto* drop = new LootItem{};
    drop->item_id = Gem;
    drop->charges = 1;
    loot.push_back(drop);
    auto* corpse = new Corpse(&appearance, &loot, Actor, &type, RuleI(NPC, MajorNPCCorpseDecayTime));
    corpse->SetEntityVariable("rof2_longshadow_corpse_source", corpse_stamp);
    for (const auto& entry : entity_list.GetClientList()) {
        const auto member = Member(*entry.second);
        if (member && member->id == run->id && member->accepted == run->accepted)
            corpse->AddLooter(entry.second);
    }
    entity_list.AddCorpse(corpse);
    corpse->Spawn();
}

} // namespace DodhBellfastHive
