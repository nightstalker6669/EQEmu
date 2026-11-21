namespace DodhBellfastKorlach {

constexpr uint32_t Task = 920273;
constexpr uint32_t Bilitan = 349940;
constexpr uint32_t Gem = 87010;
constexpr const char* Marker = "rof2_dodh_longshadow_scene_run";

const DynamicZone* Instance(Client& client)
{
    if (!zone || zone->GetZoneID() != 349 || zone->GetInstanceVersion() != 2 ||
        !zone->GetInstanceID()) return nullptr;
    const auto* mission = DodhLongshadowTravel::ActiveMission(client);
    if (!mission || mission->task != Task) return nullptr;
    const auto* dz = DodhLongshadowTravel::OwnedMission(client, *mission, false);
    return dz && dz->GetInstanceID() == zone->GetInstanceID() ? dz : nullptr;
}

std::string Scope(Client& client, const DynamicZone& dz)
{
    return fmt::format(
        "live.id={} AND live.task_id=920273 AND m.character_id={} AND dz.id={} AND i.id={} "
        "AND i.zone=349 AND i.version=2 AND UNIX_TIMESTAMP(live.accepted_time)={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP()",
        client.GetSharedTaskId(), client.CharacterID(), dz.GetID(), dz.GetInstanceID(),
        client.GetTaskState()->GetActiveSharedTask().accepted_time);
}

constexpr std::array<std::pair<uint32_t, uint32_t>, 55> TrashSpawns{{
        {261371, 349033}, {261372, 349034}, {261373, 349034}, {261374, 349035}, {261375, 349034},
        {261376, 349036}, {261377, 349036}, {261378, 349035}, {261379, 349035}, {261380, 349037},
        {261381, 349038}, {261382, 349039}, {261383, 349036}, {261384, 349040}, {261385, 349034},
        {261386, 349040}, {261387, 349036}, {261388, 349041}, {261389, 349040}, {261390, 349036},
        {261391, 349035}, {261392, 349034}, {261393, 349034}, {261394, 349040}, {261395, 349037},
        {261396, 349033}, {261397, 349039}, {261398, 349038}, {261399, 349038}, {261400, 349039},
        {261401, 349034}, {261402, 349039}, {261403, 349036}, {261404, 349036}, {261405, 349035},
        {261406, 349042}, {261407, 349039}, {261408, 349039}, {261409, 349036}, {261410, 349042},
        {261411, 349036}, {261412, 349037}, {261413, 349043}, {261414, 349038}, {261415, 349040},
        {261416, 349037}, {261417, 349039}, {261418, 349039}, {261419, 349043}, {261420, 349043},
        {261421, 349036}, {261422, 349043}, {261423, 349034}, {261424, 349037}, {261425, 349035},
}};

void RecordTrashDeath(NPC& npc, Mob* killer)
{
    const auto type = npc.GetNPCTypeID();
    const auto spawn = npc.GetSpawnPointID();
    if (!zone || zone->GetZoneID() != 349 || zone->GetInstanceVersion() != 2 ||
        type < 349033 || type > 349043 || !killer ||
        std::find(TrashSpawns.begin(), TrashSpawns.end(), std::pair{spawn, type}) == TrashSpawns.end()) return;
    auto* owner = killer->GetUltimateOwner();
    if (!owner || !owner->IsClient()) return;
    auto& client = *owner->CastToClient();
    if (!client.IsTaskActivityActive(Task, 3)) return;
    const auto* dz = Instance(client);
    if (!dz) return;

    // A respawn of the same retained spawn point cannot advance the count.
    database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_bellfast_korlach_kills "
        "(shared_task_id,accepted_time,instance_id,spawn_id,npc_type_id,killed_by) "
        "SELECT live.id,UNIX_TIMESTAMP(live.accepted_time),i.id,{},{},m.character_id "
        "FROM shared_tasks live{} WHERE {} "
        "ON DUPLICATE KEY UPDATE spawn_id=dodh_bellfast_korlach_kills.spawn_id",
        spawn, type, DodhLongshadowTravel::ScopeJoin, Scope(client, *dz)), false);
}

bool PrepareDeath(NPC& npc, Mob* killer)
{
    if (!zone || zone->GetZoneID() != 349 || zone->GetInstanceVersion() != 2 ||
        npc.GetNPCTypeID() != Bilitan) return true;
    auto* owner = killer ? killer->GetUltimateOwner() : nullptr;
    if (!owner || !owner->IsClient()) return false;
    auto& client = *owner->CastToClient();
    if (!client.IsTaskActivityActive(Task, 4) || !Instance(client) || npc.GetSpawnPointID() ||
        npc.GetEntityVariable(Marker) != std::to_string(client.GetSharedTaskId()) ||
        !database.GetItem(Gem)) return false;
    const auto& loot = npc.GetLootItems();
    if (std::none_of(loot.begin(), loot.end(),
            [](const auto* entry) { return entry && entry->item_id == Gem; }))
        npc.AddItem(Gem, 1, false);
    return true;
}

void Replay(Client& client)
{
    const bool search = client.IsTaskActivityActive(Task, 3);
    const bool fight = client.IsTaskActivityActive(Task, 4);
    if (!search && !fight && !client.IsTaskActivityActive(Task, 5)) return;
    const auto* dz = Instance(client);
    if (!dz) return;
    constexpr glm::vec4 point{-870.0f, 20.0f, -73.0f, 230.0f};
    const float z_delta = client.GetZ() - point.z;
    if (DistanceSquaredNoZ(client.GetPosition(), point) > 2500.0f || z_delta * z_delta > 400.0f) return;
    const auto* mission = DodhLongshadowTravel::Find(Task);
    const auto run = std::to_string(client.GetSharedTaskId());

    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() == Bilitan && !npc->GetDepop() &&
            npc->GetEntityVariable(Marker) == run) {
            if (search) DodhLongshadowTravel::Record(client, *mission, 3);
            return;
        }
    }
    for (const auto& entry : entity_list.GetCorpseList()) {
        if (entry.second->GetNPCTypeID() == Bilitan &&
            entry.second->GetEntityVariable("rof2_longshadow_corpse_source") == "920273:" + run) return;
    }

    auto state = database.QueryDatabase(fmt::format(
        "SELECT EXISTS(SELECT 1 FROM shared_task_activity_state a "
        "WHERE a.shared_task_id=live.id AND a.activity_id=4 AND a.done_count=1 AND a.completed_time IS NOT NULL) "
        "FROM shared_tasks live{} JOIN dodh_bellfast_korlach_settings cfg ON cfg.task_id=live.task_id "
        "WHERE {} AND (SELECT COUNT(*) FROM dodh_bellfast_korlach_kills k "
        "WHERE k.shared_task_id=live.id AND k.accepted_time=UNIX_TIMESTAMP(live.accepted_time) "
        "AND k.instance_id=i.id)>=cfg.required_kills "
        "AND NOT EXISTS(SELECT 1 FROM dodh_longshadow_item_sources s "
        "WHERE s.shared_task_id=live.id AND s.source_kind='bellfast_bilitan_gem')",
        DodhLongshadowTravel::ScopeJoin, Scope(client, *dz)), false);
    if (!state.Success() || state.RowCount() != 1 || !database.GetItem(Gem)) return;
    const auto* type = content_db.LoadNPCTypesData(Bilitan);
    if (!type) return;
    if (!Strings::ToUnsignedInt(state.begin()[0])) {
        auto* npc = new NPC(type, nullptr, point, GravityBehavior::Water);
        npc->SetEntityVariable(Marker, run);
        entity_list.AddNPC(npc);
        if (search) DodhLongshadowTravel::Record(client, *mission, 3);
        return;
    }

    NPC appearance(type, nullptr, point, GravityBehavior::Water, true);
    LootItems loot;
    auto* drop = new LootItem{};
    drop->item_id = Gem;
    drop->charges = 1;
    loot.push_back(drop);
    auto* corpse = new Corpse(&appearance, &loot, Bilitan, &type, RuleI(NPC, MajorNPCCorpseDecayTime));
    corpse->SetEntityVariable("rof2_longshadow_corpse_source", "920273:" + run);
    for (const auto& entry : entity_list.GetClientList()) {
        auto& member = *entry.second;
        if (member.GetSharedTaskId() == client.GetSharedTaskId() && dz->HasMember(member.CharacterID()))
            corpse->AddLooter(&member);
    }
    entity_list.AddCorpse(corpse);
    corpse->Spawn();
}

} // namespace DodhBellfastKorlach
