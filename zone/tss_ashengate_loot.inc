// Included by the leased native cursor writer.
struct AshengateMarkClaim {
    std::string uuid;
    uint32_t instance = 0;
    uint32_t event = 0;
    uint32_t ordinal = 0;
    std::string token;
};

bool LockAshengateMarkSource(ZoneDatabase& db, Client& client, Corpse& corpse, AshengateMarkClaim& claim)
{
    using namespace TssAshengate;
    const auto* reward = FindReward(corpse.GetNPCTypeID());
    const auto* wing = reward ? FindWing(reward->version) : nullptr;
    auto* dz = wing ? CurrentWing(*wing) : nullptr;
    if (!dz || !corpse.IsNPCCorpse() || !dz->HasMember(client.CharacterID()) ||
        corpse.GetEntityVariable(MarkSource) != fmt::format("{}:{}", dz->GetUUID(), reward->event)) return false;

    auto binding = dz->GetLootEvent(corpse.GetID(), DzLootEvent::Type::Entity);
    if (binding.empty()) binding = dz->GetLootEvent(reward->npc, DzLootEvent::Type::NpcType);
    if (binding != reward->name) return false;

    const auto uuid = db.Escape(dz->GetUUID());
    const auto event = db.Escape(reward->name);
    const auto id = client.CharacterID();
    auto engines = Q(db,
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
        "AND table_name IN ('tss_ashengate_event_credit','dynamic_zones','dynamic_zone_members',"
        "'instance_list','instance_list_player','dynamic_zone_lockouts','character_expedition_lockouts')");
    if (engines.RowCount() != 1 || N(engines.begin()[0]) != 7) return false;

    auto owned = Q(db, fmt::format(
        "SELECT d.id FROM dynamic_zones d JOIN instance_list i ON i.id=d.instance_id "
        "JOIN dynamic_zone_members m ON m.dynamic_zone_id=d.id AND m.character_id={0} "
        "JOIN instance_list_player p ON p.id=i.id AND p.charid={0} "
        "WHERE d.id={1} AND d.uuid='{2}' AND d.type={3} AND BINARY d.name='{4}' "
        "AND i.id={5} AND i.zone=406 AND i.version={6} AND i.expire_at>UNIX_TIMESTAMP() FOR UPDATE",
        id, dz->GetID(), uuid, static_cast<uint32_t>(DynamicZoneType::Expedition), wing->name,
        dz->GetInstanceID(), wing->version));
    if (owned.RowCount() != 1) return false;

    auto earned = Q(db, fmt::format(
        "SELECT c.character_id FROM tss_ashengate_event_credit c "
        "JOIN dynamic_zone_lockouts d ON d.dynamic_zone_id={0} AND BINARY d.event_name='{1}' "
        "AND d.from_expedition_uuid=c.dz_uuid AND d.duration=388800 AND d.expire_time>NOW() "
        "JOIN character_expedition_lockouts l ON l.character_id=c.character_id AND BINARY l.event_name='{1}' "
        "AND BINARY l.expedition_name='{2}' AND l.from_expedition_uuid=c.dz_uuid "
        "AND l.duration=388800 AND l.expire_time>NOW() "
        "WHERE c.dz_uuid='{3}' AND c.event_id={4} AND c.character_id={5} "
        "AND c.instance_id={6} AND c.earned_at>0 FOR UPDATE",
        dz->GetID(), event, wing->name, uuid, reward->event, id, dz->GetInstanceID()));
    if (earned.RowCount() != 1) return false;
    auto issued = Q(db, fmt::format(
        "SELECT source_ordinal,instance_id FROM tss_ashengate_mark_loot "
        "WHERE dz_uuid='{}' AND event_id={} ORDER BY source_ordinal FOR UPDATE", uuid, reward->event));
    uint32_t count = 0;
    for (auto row = issued.begin(); row != issued.end(); ++row) {
        if (N(row[0]) != ++count || N(row[1]) != dz->GetInstanceID() || count > reward->mark_count)
            return false;
    }
    claim = {dz->GetUUID(), dz->GetInstanceID(), reward->event,
             count == reward->mark_count ? 0 : count + 1, {}};
    return true;
}
