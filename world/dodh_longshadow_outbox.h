#include "common/dodh_longshadow_travel.h"
#pragma once

#include "common/shared_core/dodh_longshadow_journal.h"
#include "common/shared_core/dodh_longshadow_sources.h"
#include "common/shared_task_admission.h"
#include <fmt/format.h>
#include <algorithm>
#include <vector>

namespace DodhLongshadowOutbox {
namespace Policy = EQEmu::SharedCore::DodhLongshadowCustody;

struct Pending {
    uint64_t run;
    uint32_t task;
    uint32_t activity;
    uint32_t character;
};

// The generic writer's credit is eligible only after durable inventory
// settlement and only for the receipt belonging to this exact task run.
inline constexpr const char* ReceiptJoin =
    " FROM dodh_longshadow_turnins t "
    "JOIN dodh_longshadow_item_sources s ON s.item_guid=t.item_guid AND s.shared_task_id=t.shared_task_id "
    "AND s.task_id=t.task_id AND s.character_id=t.character_id "
    "JOIN kithicor_poison_custody w ON w.request_key=t.request_key AND w.shared_task_id=t.shared_task_id "
    "AND w.character_id=t.character_id AND w.source_guid=COALESCE(s.inventory_guid,s.item_guid) AND w.physical=1 AND w.settled=1 "
    "AND w.abort_only=0 AND w.recipe_kind=4 AND LEFT(w.recipe_context,6)=X'444F44480104' ";
inline constexpr const char* LiveJoin =
    " JOIN shared_tasks live ON live.id=t.shared_task_id AND live.task_id=t.task_id "
    "AND UNIX_TIMESTAMP(live.accepted_time)=s.accepted_time "
    "JOIN shared_task_members member ON member.shared_task_id=live.id AND member.character_id=t.character_id ";

inline constexpr const char* SceneJoin =
    "JOIN dodh_longshadow_scene_effects e ON e.request_key=t.request_key AND e.shared_task_id=t.shared_task_id "
    "AND e.task_id=t.task_id AND e.activity_id=t.activity_id AND e.character_id=t.character_id "
    "AND e.source_item_guid=t.item_guid AND e.instance_id=s.instance_id ";

inline bool Permits(Database& db, const Pending& record)
{
    const auto* route = Policy::FindRoute(record.task, record.activity);
    if (!route) return false;
    const auto actor = Policy::SceneActor(route->effect);
    auto result = db.QueryDatabase(fmt::format(
        "SELECT 1{}{}{}WHERE t.shared_task_id={} AND t.task_id={} AND t.activity_id={} "
        "AND t.character_id={} AND s.item_id={} AND s.source_kind='{}' AND live.completion_time IS NULL "
        "AND (live.expire_time IS NULL OR live.expire_time>NOW()){}",
        ReceiptJoin, actor ? SceneJoin : "", LiveJoin, record.run, record.task, record.activity,
        record.character, route->item, Policy::SourceKind(route->source),
        actor ? fmt::format(" AND e.actor_type={} AND e.materialized=1", actor) : std::string{}), false);
    return result.Success() && result.RowCount() == 1;
}

inline std::vector<Pending> ReadPending(Database& db)
{
    auto result = db.QueryDatabase(fmt::format(
        "SELECT t.shared_task_id,t.task_id,t.activity_id,t.character_id{}WHERE t.effect_applied=0 "
        "AND (EXISTS(SELECT 1 FROM shared_tasks live JOIN shared_task_members m "
        "ON m.shared_task_id=live.id AND m.character_id=t.character_id "
        "WHERE live.id=t.shared_task_id AND live.task_id=t.task_id "
        "AND UNIX_TIMESTAMP(live.accepted_time)=s.accepted_time "
        "AND (live.expire_time IS NULL OR live.expire_time>NOW())) OR "
        "EXISTS(SELECT 1 FROM completed_shared_tasks finished JOIN completed_shared_task_members m "
        "ON m.shared_task_id=finished.id AND m.character_id=t.character_id "
        "WHERE finished.id=t.shared_task_id AND finished.task_id=t.task_id "
        "AND UNIX_TIMESTAMP(finished.accepted_time)=s.accepted_time AND finished.completion_time IS NOT NULL)) "
        "ORDER BY t.shared_task_id,t.activity_id", ReceiptJoin), false);
    std::vector<Pending> pending;
    if (!result.Success()) return pending;
    for (auto row : result) {
        const auto run = SharedTaskAdmission::Number(row[0]);
        const auto task = SharedTaskAdmission::Number(row[1]);
        const auto activity = SharedTaskAdmission::Number(row[2]);
        const auto character = SharedTaskAdmission::Number(row[3]);
        if (!run || !*run || !task || *task > UINT32_MAX || !activity || *activity > UINT32_MAX ||
            !character || !*character || *character > UINT32_MAX) continue;
        const auto* route = Policy::FindRoute(uint32_t(*task), uint32_t(*activity));
        if (route)
            pending.push_back({*run, uint32_t(*task), uint32_t(*activity), uint32_t(*character)});
    }
    return pending;
}

inline constexpr const char* SourceJoin =
    " FROM dodh_longshadow_item_sources source "
    "JOIN shared_tasks live ON live.id=source.shared_task_id AND live.task_id=source.task_id "
    "AND UNIX_TIMESTAMP(live.accepted_time)=source.accepted_time "
    "JOIN shared_task_members member ON member.shared_task_id=live.id AND member.character_id=source.character_id "
    "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=live.id "
    "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 AND dz.instance_id=source.instance_id "
    "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=member.character_id "
    "WHERE live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) ";

inline constexpr const char* ShadowspinDeathJoin =
    " FROM dodh_longshadow_scene_effects e "
    "JOIN dodh_longshadow_turnins t ON t.request_key=e.request_key AND t.shared_task_id=e.shared_task_id "
    "AND t.task_id=e.task_id AND t.activity_id=e.activity_id AND t.character_id=e.character_id AND t.item_guid=e.source_item_guid "
    "JOIN dodh_longshadow_item_sources s ON s.item_guid=e.source_item_guid AND s.shared_task_id=e.shared_task_id "
    "AND s.task_id=e.task_id AND s.character_id=e.character_id AND s.instance_id=e.instance_id "
    "JOIN kithicor_poison_custody w ON w.request_key=t.request_key AND w.shared_task_id=t.shared_task_id "
    "AND w.character_id=t.character_id AND w.source_guid=COALESCE(s.inventory_guid,s.item_guid) AND w.physical=1 AND w.settled=1 "
    "AND w.abort_only=0 AND w.recipe_kind=4 AND LEFT(w.recipe_context,6)=X'444F44480104' "
    "JOIN shared_tasks live ON live.id=e.shared_task_id AND live.task_id=e.task_id "
    "AND UNIX_TIMESTAMP(live.accepted_time)=s.accepted_time "
    "JOIN shared_task_members member ON member.shared_task_id=live.id AND member.character_id=e.death_character_id "
    "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=live.id "
    "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 AND dz.instance_id=e.instance_id "
    "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=member.character_id "
    "JOIN instance_list i ON i.id=dz.instance_id AND i.zone=354 AND i.version=1 "
    "WHERE live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
    "AND e.task_id=920251 AND e.activity_id=1 AND e.actor_type=354920 AND e.materialized=1 "
    "AND e.death_character_id>0 AND e.loot_item_id=54607 AND s.item_id=54606 AND s.source_kind='dustspirit_note' ";

inline constexpr const char* TravelJoin =
    " JOIN shared_tasks live ON live.id=e.shared_task_id AND live.task_id=e.task_id "
    " AND UNIX_TIMESTAMP(live.accepted_time)=e.accepted_time ";

inline bool PermitsTravel(Database& db, const Pending& record)
{
    const auto* mission = DodhLongshadowTravel::Find(record.task);
    if (!mission || !DodhLongshadowTravel::OwnsActivity(record.task, record.activity)) return false;
    const bool neran = DodhLongshadowTravel::NeranPublicActivity(record.task, record.activity);
    const char* witness = !neran ? "" : record.activity == 0 ?
        " AND EXISTS (SELECT 1 FROM dodh_bellfast_neran_runs n WHERE n.shared_task_id=e.shared_task_id "
        "AND n.accepted_time=e.accepted_time AND n.instance_id=e.instance_id)" :
        " AND EXISTS (SELECT 1 FROM dodh_bellfast_neran_crossings n JOIN dodh_bellfast_neran_runs scene "
        "ON scene.shared_task_id=n.shared_task_id AND scene.accepted_time=n.accepted_time "
        "AND scene.instance_id=n.instance_id AND scene.phase=2 WHERE n.shared_task_id=e.shared_task_id "
        "AND n.accepted_time=e.accepted_time AND n.instance_id=e.instance_id AND n.character_id=e.character_id "
        "AND n.spell_id=8403)";
    const auto result = db.QueryDatabase(fmt::format(
        "SELECT 1 FROM dodh_longshadow_travel_events e{}{}"
        "WHERE e.shared_task_id={} AND e.task_id={} AND e.activity_id={} AND e.character_id={} "
        "AND m.character_id=e.character_id AND i.id=e.instance_id AND i.zone={} AND i.version={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP(){}",
        TravelJoin, DodhLongshadowTravel::ScopeJoin, record.run, record.task, record.activity,
        record.character, mission->zone, mission->version, witness), false);
    return result.Success() && result.RowCount() == 1;
}

inline constexpr const char* OrbDeathJoin =
    " JOIN shared_tasks live ON live.id=d.shared_task_id AND live.task_id=920252 "
    " AND UNIX_TIMESTAMP(live.accepted_time)=d.accepted_time ";

inline bool PermitsOrbDeath(Database& db, const Pending& record)
{
    const auto result = db.QueryDatabase(fmt::format(
        "SELECT 1 FROM dodh_longshadow_orb_deaths d{}{}"
        "WHERE d.shared_task_id={} AND m.character_id={} AND i.id=d.instance_id "
        "AND i.zone=354 AND i.version=5 AND d.actor_type=354311 AND d.spawn_id=262895 "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP()",
        OrbDeathJoin, DodhLongshadowTravel::ScopeJoin, record.run, record.character), false);
    return result.Success() && result.RowCount() == 1;
}

inline constexpr const char* TraitorNativeOpen =
    " AND EXISTS (SELECT 1 FROM dodh_longshadow_traitor_chests chest "
    "WHERE chest.shared_task_id=source.shared_task_id AND chest.accepted_time=source.accepted_time "
    "AND chest.instance_id=source.instance_id AND chest.opened_by IS NOT NULL) ";

inline bool PermitsSource(Database& db, const Pending& record)
{
    if (DodhLongshadowTravel::OrbDeath(record.task, record.activity))
        return PermitsOrbDeath(db, record);
    if (DodhLongshadowTravel::OwnsActivity(record.task, record.activity))
        return PermitsTravel(db, record);
    if (Policy::DeathActivity(record.task, record.activity)) {
        const auto result = db.QueryDatabase(fmt::format(
            "SELECT 1{}AND e.shared_task_id={} AND e.death_character_id={}",
            ShadowspinDeathJoin, record.run, record.character), false);
        return result.Success() && result.RowCount() == 1;
    }
    const auto* route = Policy::FindSourceActivity(record.task, record.activity);
    if (!route) return false;
    auto result = db.QueryDatabase(fmt::format(
        "SELECT 1{}AND live.id={} AND live.task_id={} AND source.character_id={} "
        "AND source.item_id={} AND source.source_kind='{}' AND source.item_guid>0{}",
        SourceJoin, record.run, record.task, record.character, route->item, Policy::SourceKind(route->source),
        route->source == Policy::Source::TraitorChest ? TraitorNativeOpen : ""), false);
    return result.Success() && result.RowCount() == 1;
}

inline std::vector<Pending> ReadPendingSources(Database& db)
{
    std::vector<Pending> pending;
    for (const auto& route : Policy::Sources) {
        if (!route.publishes_activity) continue;
        auto result = db.QueryDatabase(fmt::format(
            "SELECT source.shared_task_id,source.character_id{}AND source.task_id={} "
            "AND source.item_id={} AND source.source_kind='{}' AND source.item_guid>0 "
            "AND NOT EXISTS (SELECT 1 FROM shared_task_activity_state a "
            "WHERE a.shared_task_id=live.id AND a.activity_id={} "
            "AND a.done_count=1 AND a.completed_time IS NOT NULL)",
            SourceJoin, route.task, route.item, Policy::SourceKind(route.source), route.activity), false);
        if (!result.Success()) continue;
        for (auto row : result) {
            const auto run = SharedTaskAdmission::Number(row[0]), character = SharedTaskAdmission::Number(row[1]);
            if (run && *run && character && *character && *character <= UINT32_MAX)
                pending.push_back({*run, route.task, route.activity, uint32_t(*character)});
        }
    }
    auto deaths = db.QueryDatabase(fmt::format(
        "SELECT e.shared_task_id,e.death_character_id{}AND NOT EXISTS "
        "(SELECT 1 FROM shared_task_activity_state a WHERE a.shared_task_id=e.shared_task_id "
        "AND a.activity_id=2 AND a.done_count=1 AND a.completed_time IS NOT NULL)",
        ShadowspinDeathJoin), false);
    if (deaths.Success()) {
        for (auto row : deaths) {
            const auto run = SharedTaskAdmission::Number(row[0]), character = SharedTaskAdmission::Number(row[1]);
            if (run && *run && character && *character && *character <= UINT32_MAX)
                pending.push_back({*run, 920251, 2, uint32_t(*character)});
        }
    }
    auto travel = db.QueryDatabase(fmt::format(
        "SELECT e.shared_task_id,e.task_id,e.activity_id,e.character_id "
        "FROM dodh_longshadow_travel_events e{} "
        "WHERE live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND NOT EXISTS (SELECT 1 FROM shared_task_activity_state a "
        "WHERE a.shared_task_id=e.shared_task_id AND a.activity_id=e.activity_id "
        "AND a.done_count=1 AND a.completed_time IS NOT NULL)",
        TravelJoin), false);
    if (travel.Success()) {
        for (auto row : travel) {
            const auto run = SharedTaskAdmission::Number(row[0]), task = SharedTaskAdmission::Number(row[1]);
            const auto activity = SharedTaskAdmission::Number(row[2]), character = SharedTaskAdmission::Number(row[3]);
            if (run && *run && task && *task <= UINT32_MAX && activity && *activity <= UINT32_MAX &&
                character && *character && *character <= UINT32_MAX &&
                DodhLongshadowTravel::OwnsActivity(uint32_t(*task), uint32_t(*activity)))
                pending.push_back({*run, uint32_t(*task), uint32_t(*activity), uint32_t(*character)});
        }
    }
    auto orb = db.QueryDatabase(fmt::format(
        "SELECT d.shared_task_id,MIN(m.character_id) FROM dodh_longshadow_orb_deaths d{}{}"
        "WHERE live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.id=d.instance_id AND i.zone=354 AND i.version=5 "
        "AND NOT EXISTS (SELECT 1 FROM shared_task_activity_state a "
        "WHERE a.shared_task_id=d.shared_task_id AND a.activity_id=1 "
        "AND a.done_count=1 AND a.completed_time IS NOT NULL) GROUP BY d.shared_task_id",
        OrbDeathJoin, DodhLongshadowTravel::ScopeJoin), false);
    if (orb.Success()) {
        for (auto row : orb) {
            const auto run = SharedTaskAdmission::Number(row[0]), character = SharedTaskAdmission::Number(row[1]);
            if (run && *run && character && *character && *character <= UINT32_MAX)
                pending.push_back({*run, 920252, 1, uint32_t(*character)});
        }
    }
    return pending;
}

// A missing acknowledgement remains retryable. The caller never applies a
// relative increment. World owns the public native task counter.
inline bool MarkObserved(Database& db, const Pending& record)
{
    auto result = db.QueryDatabase(fmt::format(
        "UPDATE dodh_longshadow_turnins t SET t.effect_applied=1 "
        "WHERE t.shared_task_id={} AND t.task_id={} AND t.activity_id={} AND t.character_id={} "
        "AND t.effect_applied=0 AND ("
          "EXISTS(SELECT 1 FROM shared_task_activity_state a WHERE a.shared_task_id=t.shared_task_id "
          "AND a.activity_id=t.activity_id AND a.done_count=1 AND a.completed_time IS NOT NULL) OR "
          "EXISTS(SELECT 1 FROM completed_shared_task_activity_state a "
          "JOIN completed_shared_tasks c ON c.id=a.shared_task_id AND c.task_id=t.task_id "
          "WHERE a.shared_task_id=t.shared_task_id AND a.activity_id=t.activity_id "
          "AND a.done_count=1 AND a.completed_time IS NOT NULL AND c.completion_time IS NOT NULL))",
        record.run, record.task, record.activity, record.character), false);
    return result.Success() && result.RowsAffected() == 1;
}
}
