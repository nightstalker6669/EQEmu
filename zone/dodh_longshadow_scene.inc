// Included once by the leased trade writer. Only native combat creates death receipts.
#include "corpse.h"
#include "dodh_cocoon_open_receipt.h"
#include "zone.h"
#include <cmath>
#include <cstdlib>

namespace DodhLongshadowScene {
namespace Policy = EQEmu::SharedCore::DodhLongshadowCustody;

constexpr const char* Marker = "rof2_dodh_longshadow_effect";
constexpr const char* RunMarker = "rof2_dodh_longshadow_scene_run";
constexpr const char* Phase = "rof2_dodh_shadowspin_phase";

constexpr const char* ScopeJoin =
    " JOIN dodh_longshadow_turnins t ON t.request_key=e.request_key AND t.shared_task_id=e.shared_task_id "
    "AND t.task_id=e.task_id AND t.activity_id=e.activity_id AND t.character_id=e.character_id AND t.item_guid=e.source_item_guid "
    "JOIN dodh_longshadow_item_sources source ON source.item_guid=e.source_item_guid AND source.character_id=e.character_id "
    "AND source.shared_task_id=e.shared_task_id AND source.task_id=e.task_id AND source.instance_id=e.instance_id "
    "JOIN kithicor_poison_custody w ON w.request_key=e.request_key AND w.shared_task_id=e.shared_task_id "
    "AND w.character_id=e.character_id AND w.source_guid=COALESCE(source.inventory_guid,source.item_guid) AND w.physical=1 AND w.settled=1 "
    "AND w.abort_only=0 AND w.recipe_kind=4 AND LEFT(w.recipe_context,6)=X'444F44480104' "
    "JOIN shared_tasks live ON live.id=e.shared_task_id AND live.task_id=e.task_id "
    "AND UNIX_TIMESTAMP(live.accepted_time)=source.accepted_time AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
    "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=live.id "
    "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 AND dz.instance_id=e.instance_id "
    "JOIN instance_list i ON i.id=dz.instance_id ";

struct Identity {
    uint64_t run;
    std::string key;

    std::string Stamp() const { return fmt::format("{}:{}", run, key); }
};

std::optional<Identity> ReadIdentity(Mob& actor)
{
    const auto stamp = actor.GetEntityVariable(Marker);
    const auto colon = stamp.find(':');
    if (colon == std::string::npos) return std::nullopt;
    const auto run = SharedTaskAdmission::Number(stamp.substr(0, colon).c_str());
    auto key = stamp.substr(colon + 1);
    if (!run || !*run || !DodhMissionCustody::KnownRequestKey(key)) return std::nullopt;
    return Identity{*run, std::move(key)};
}

const Policy::Route* RouteForActor(uint32_t actor)
{
    for (const auto& route : Policy::NormalRoutes) {
        if (actor && actor == Policy::SceneActor(route.effect)) return &route;
    }
    return nullptr;
}

bool InScope(const Policy::Route& route)
{
    return zone && zone->GetInstanceID() && zone->GetZoneID() == route.zone &&
        zone->GetInstanceVersion() == route.version && zone->GetDynamicZone();
}

std::string ExactScope(const Policy::Route& route, const Identity& id)
{
    return fmt::format(
        "e.request_key='{}' AND e.shared_task_id={} AND e.task_id={} AND e.activity_id={} AND e.actor_type={} "
        "AND e.instance_id={} AND dz.id={} AND i.zone={} AND i.version={} AND source.item_id={} AND source.source_kind='{}'",
        id.key, id.run, route.task, route.activity, Policy::SceneActor(route.effect),
        zone->GetInstanceID(), zone->GetDynamicZone()->GetID(), route.zone, route.version, route.item,
        Policy::SourceKind(route.source));
}

std::string MemberWhere(uint32_t character)
{
    return fmt::format(
        "EXISTS (SELECT 1 FROM shared_task_members m JOIN dynamic_zone_members dm "
        "ON dm.dynamic_zone_id=dz.id AND dm.character_id=m.character_id "
        "WHERE m.shared_task_id=e.shared_task_id AND m.character_id={})", character);
}

bool Member(Client& client, const Identity& id)
{
    return client.GetSharedTaskId() > 0 && uint64_t(client.GetSharedTaskId()) == id.run &&
        zone->GetDynamicZone()->HasMember(client.CharacterID());
}

Client* ResponsiblePlayer(Mob* attacker)
{
    if (!attacker) return nullptr;
    auto* owner = attacker->GetUltimateOwner();
    return owner && owner->IsClient() ? owner->CastToClient() : nullptr;
}

int64 CapDamage(NPC& npc, Mob* attacker, int64 amount)
{
    const auto* route = RouteForActor(npc.GetNPCTypeID());
    if (!route || amount <= 0 || !InScope(*route)) return amount;
    auto* client = ResponsiblePlayer(attacker);
    const auto id = ReadIdentity(npc);
    if (!id) return amount;
    if (!client || !Member(*client, *id)) return 0;
    if (route->effect == Policy::Effect::OpenBloodeyeChest) return 0;
    if (!client->IsTaskActivityActive(route->task, 2)) return 0;

    constexpr std::array<int, 3> thresholds{75, 50, 25};
    const auto stage = SharedTaskAdmission::Number(npc.GetEntityVariable(Phase).c_str()).value_or(0);
    if (stage >= thresholds.size()) return amount;
    const auto remaining = npc.GetHP() - npc.GetMaxHP() * thresholds[stage] / 100;
    if (amount < remaining) return amount;
    const auto* type = content_db.LoadNPCTypesData(354921);
    if (!type) return 0;
    auto* add = new NPC(type, nullptr, npc.GetPosition(), GravityBehavior::Water);
    add->SetEntityVariable("rof2_dodh_shadowspin_add", id->Stamp());
    entity_list.AddNPC(add, true, true);
    add->AddToHateList(client, 1, 0, false);
    npc.SetEntityVariable(Phase, std::to_string(stage + 1));
    return std::max<int64>(0, remaining);
}

bool RecordDeath(NPC& npc, Mob* attacker)
{
    const auto* route = RouteForActor(npc.GetNPCTypeID());
    if (!route || !InScope(*route)) return true;
    const auto id = ReadIdentity(npc);
    auto* client = ResponsiblePlayer(attacker);
    if (!id) return true;
    if (!client || !Member(*client, *id) || npc.GetDepop()) return false;
    if (route->effect == Policy::Effect::OpenBloodeyeChest &&
        !DodhCocoonOpen::ScopedOpen::Matches(&npc, attacker)) return false;
    if (route->effect == Policy::Effect::SpawnShadowspin &&
        (!client->IsTaskActivityActive(route->task, 2) || npc.GetEntityVariable(Phase) != "3")) return false;

    const auto& loot = npc.GetLootItems();
    if (loot.size() != 1 || !loot.front() || !Policy::SceneLoot(npc.GetNPCTypeID(), loot.front()->item_id))
        return false;
    const auto item = loot.front()->item_id;
    const auto& point = npc.GetPosition();
    const auto exact = ExactScope(*route, *id);
    const auto member = MemberWhere(client->CharacterID());
    const auto result = database.QueryDatabase(fmt::format(
        "UPDATE dodh_longshadow_scene_effects e{}SET e.death_character_id={},e.loot_item_id={},"
        "e.death_x={},e.death_y={},e.death_z={},e.death_heading={} WHERE {} AND {} "
        "AND e.materialized=1 AND e.terminal=0 AND e.death_character_id IS NULL AND e.prepared_item_id={}",
        ScopeJoin, client->CharacterID(), item, point.x, point.y, point.z, point.w, exact, member, item), false);
    if (result.Success() && result.RowsAffected() == 1) return true;
    // An ambiguous write only proceeds on exact durable readback.
    const auto found = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM dodh_longshadow_scene_effects e{}WHERE {} AND {} AND e.materialized=1 "
        "AND e.terminal=0 AND e.death_character_id={} AND e.loot_item_id={}",
        ScopeJoin, exact, member, client->CharacterID(), item), false);
    return found.Success() && found.RowCount() == 1;
}

void StampCorpse(NPC& npc, Corpse& corpse)
{
    const auto* route = RouteForActor(npc.GetNPCTypeID());
    const auto id = ReadIdentity(npc);
    if (route && id && InScope(*route)) corpse.SetEntityVariable(Marker, id->Stamp());
}

// Shared by live and recovered corpses. The normal permission check still
// runs first. The cursor writer repeats the durable membership test on debit.
bool AllowPleaOpen(Client& client, NPC& chest)
{
    const auto* route = RouteForActor(chest.GetNPCTypeID());
    const auto id = ReadIdentity(chest);
    return route && route->effect == Policy::Effect::OpenBloodeyeChest &&
        id && InScope(*route) && Member(client, *id);
}

bool AllowLoot(Client& client, Corpse& corpse)
{
    const auto* route = RouteForActor(corpse.GetNPCTypeID());
    const auto id = ReadIdentity(corpse);
    return route && id && InScope(*route) && Member(client, *id);
}

bool ReplayOwned(Mob& actor)
{
    if (!zone || !zone->GetInstanceID() || zone->GetInstanceVersion() != 1) return false;
    const auto* route = RouteForActor(actor.GetNPCTypeID());
    return (route && InScope(*route) && ReadIdentity(actor).has_value()) ||
        (actor.GetNPCTypeID() == 354921 && zone->GetZoneID() == 354 &&
         !actor.GetEntityVariable("rof2_dodh_shadowspin_add").empty());
}

std::optional<glm::vec4> Point(char** values)
{
    glm::vec4 point;
    for (int i = 0; i < 4; ++i) {
        if (!values[i]) return std::nullopt;
        char* end = nullptr;
        point[i] = std::strtof(values[i], &end);
        if (end == values[i] || *end || !std::isfinite(point[i])) return std::nullopt;
    }
    return point;
}

void Replay()
{
    if (!zone || !zone->GetInstanceID() || zone->GetInstanceVersion() != 1) return;
    const auto* route = RouteForActor(zone->GetZoneID() == 354 ? 354920 : zone->GetZoneID() == 363 ? 363035 : 0);
    if (!route || !InScope(*route) || entity_list.GetClientList().empty()) return;

    const auto type_id = Policy::SceneActor(route->effect);
    auto query = database.QueryDatabase(fmt::format(
        "SELECT e.shared_task_id,e.request_key,e.materialized,e.death_character_id,e.loot_item_id,"
        "e.death_x,e.death_y,e.death_z,e.death_heading,f.x,f.y,f.z,f.heading,f.loottable_id,e.prepared_item_id "
        "FROM dodh_longshadow_scene_effects e{}LEFT JOIN dodh_longshadow_scene_fixtures f "
        "ON f.actor_type=e.actor_type AND f.zone_id=i.zone AND f.zone_version=i.version "
        "WHERE e.task_id={} AND e.activity_id={} AND e.actor_type={} AND e.instance_id={} AND dz.id={} "
        "AND i.zone={} AND i.version={} AND e.terminal=0 AND source.item_id={} AND source.source_kind='{}'",
        ScopeJoin, route->task, route->activity, type_id, zone->GetInstanceID(), zone->GetDynamicZone()->GetID(),
        route->zone, route->version, route->item, Policy::SourceKind(route->source)), false);
    if (!query.Success() || query.RowCount() != 1) return;
    auto row = query.begin();
    const auto run = SharedTaskAdmission::Number(row[0]).value_or(0);
    if (!run || !row[1] || !DodhMissionCustody::KnownRequestKey(row[1])) return;
    const Identity id{run, row[1]};
    if (std::none_of(entity_list.GetClientList().begin(), entity_list.GetClientList().end(),
        [&](const auto& entry) { return Member(*entry.second, id); })) return;
    const auto stamp = id.Stamp();
    const auto* type = content_db.LoadNPCTypesData(type_id);
    if (!type) return;
    const bool materialized = SharedTaskAdmission::Number(row[2]).value_or(0) == 1;
    const auto killed = SharedTaskAdmission::Number(row[3]).value_or(0);
    const auto item = SharedTaskAdmission::Number(row[4]).value_or(0);

    NPC* current = nullptr;
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() != type_id || npc->GetDepop()) continue;
        if (current || npc->GetEntityVariable(Marker) != stamp) return;
        current = npc;
    }
    if (killed) {
        if (current) current->Depop(false);
        for (const auto& entry : entity_list.GetCorpseList()) {
            auto* corpse = entry.second;
            if (corpse->GetNPCTypeID() == type_id && corpse->GetEntityVariable(Marker) == stamp) return;
        }
        char* location[] = {row[5], row[6], row[7], row[8]};
        const auto point = Point(location);
        if (!point || item > UINT32_MAX || !Policy::SceneLoot(type_id, uint32_t(item)) ||
            !database.GetItem(uint32_t(item))) return;
        NPC appearance(type, nullptr, *point, GravityBehavior::Water, true);
        LootItems loot;
        auto* drop = new LootItem{};
        drop->item_id = uint32_t(item);
        drop->charges = 1;
        loot.push_back(drop);
        auto* corpse = new Corpse(&appearance, &loot, type_id, &type, RuleI(NPC, MajorNPCCorpseDecayTime));
        corpse->SetEntityVariable(Marker, stamp);
        corpse->SetEntityVariable("rof2_longshadow_corpse_source", fmt::format("{}:{}", route->task, run));
        for (const auto& entry : entity_list.GetClientList()) {
            if (Member(*entry.second, id)) corpse->AddLooter(entry.second);
        }
        entity_list.AddCorpse(corpse);
        corpse->Spawn();
        return;
    }
    if (!current) {
        char* location[] = {row[9], row[10], row[11], row[12]};
        const auto point = Point(location);
        const auto table = SharedTaskAdmission::Number(row[13]).value_or(0);
        if (!point || table > UINT32_MAX || (bool(table) != (type_id == 363035))) return;
        auto npc = std::make_unique<NPC>(type, nullptr, *point, GravityBehavior::Water);
        auto prepared = SharedTaskAdmission::Number(row[14]).value_or(0);
        if (prepared || !table) {
            if (!table) prepared = 54607;
            if (prepared > UINT32_MAX || !Policy::SceneLoot(type_id, uint32_t(prepared))) return;
            npc->AddItem(uint32_t(prepared), 1, false);
        } else {
            npc->AddLootTable(uint32_t(table));
        }
        const auto& loot = npc->GetLootItems();
        if (loot.size() != 1 || !loot.front() || !Policy::SceneLoot(type_id, loot.front()->item_id)) return;
        if (!row[14]) {
            const auto committed = database.QueryDatabase(fmt::format(
                "UPDATE dodh_longshadow_scene_effects e{}SET e.prepared_item_id={} "
                "WHERE {} AND e.terminal=0 AND e.death_character_id IS NULL AND e.prepared_item_id IS NULL",
                ScopeJoin, loot.front()->item_id, ExactScope(*route, id)), false);
            // On an unknown write the next replay reads the prepared item
            // before showing a chest to anybody.
            if (!committed.Success() || committed.RowsAffected() != 1) return;
        }
        npc->SetEntityVariable(Marker, stamp);
        npc->SetEntityVariable(RunMarker, std::to_string(run));
        current = npc.release();
        entity_list.AddNPC(current, true, true);
    }
    if (!materialized && !current->GetDepop()) {
        database.QueryDatabase(fmt::format(
            "UPDATE dodh_longshadow_scene_effects e{}SET e.materialized=1 "
            "WHERE {} AND e.materialized=0 AND e.terminal=0 AND e.prepared_item_id IS NOT NULL",
            ScopeJoin, ExactScope(*route, id)), false);
    }
}
}
