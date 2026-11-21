#pragma once

#include "common/shared_core/dodh_longshadow_sources.h"
#include "zonedb.h"
#include <fmt/format.h>
#include <algorithm>
#include <string_view>

namespace DodhMissionCustody {
namespace Policy = EQEmu::SharedCore::DodhLongshadowCustody;

using Policy::SourceKind;

struct MissionPlacement {
    uint32_t zone;
    uint32_t version;
};

inline std::optional<MissionPlacement> Placement(uint32_t task)
{
    switch (task) {
    case 920251: return MissionPlacement{354, 1};
    case 920252: return MissionPlacement{354, 5};
    case 920253: return MissionPlacement{363, 1};
    case 920255: return MissionPlacement{363, 3};
    case 920270: return MissionPlacement{367, 1};
    case 920271: return MissionPlacement{360, 1};
    case 920272: return MissionPlacement{355, 1};
    case 920273: return MissionPlacement{349, 2};
    default: return std::nullopt;
    }
}

inline bool WriterTransaction(ZoneDatabase& db)
{
    auto query = db.QueryDatabase(std::string{"SELECT @@in_transaction = 1"}, false);
    return query.Success() && query.RowCount() == 1 && query.begin()[0] &&
        std::string_view(query.begin()[0]) == "1";
}

inline bool KnownRequestKey(std::string_view key)
{
    return key.size() == 32 && key.find_first_not_of("0123456789ABCDEFabcdef") == key.npos;
}

// The caller supplies the native writer's current task, scope and four
// captured offer roots. Reuse the existing game's inventory transaction.
struct LockedDebit {
    Policy::SourceReceipt source;
    Policy::Trade::DebitPlan debit;
    ZoneDatabase* connection;
    uint32_t original_guid;
};

inline std::optional<LockedDebit> LockTurnin(
    ZoneDatabase& db, const Policy::Route& route, const Policy::Context& context,
    const Policy::Trade::Offers& offers)
{
    const auto* kind = SourceKind(route.source);
    const auto placement = Placement(route.task);
    if (!kind || !placement || !WriterTransaction(db)) return std::nullopt;
    for (const auto& offer : offers) {
        if (!offer.present || offer.item_id != route.item || offer.guid <= 0) continue;
        const Policy::SourceReceipt receipt{
            context.occurrence, context.accepted_time, context.character, route.task,
            context.mission_instance, route.item, offer.guid, route.source
        };
        auto debit = Policy::Plan(route, context, receipt, offers);
        if (!debit) return std::nullopt;
        auto query = db.QueryDatabase(fmt::format(
            "SELECT source.item_guid FROM dodh_longshadow_item_sources source "
            "JOIN shared_tasks task ON task.id=source.shared_task_id AND task.task_id=source.task_id "
            "JOIN shared_task_members member ON member.shared_task_id=task.id "
            " AND member.character_id=source.character_id "
            "JOIN shared_task_activity_state activity ON activity.shared_task_id=task.id "
            " AND activity.activity_id={} "
            "JOIN shared_task_dynamic_zones link ON link.shared_task_id=task.id "
            "JOIN dynamic_zones dz ON dz.id=link.dynamic_zone_id AND dz.instance_id=source.instance_id "
            "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=member.character_id "
            "JOIN instance_list inst ON inst.id=dz.instance_id AND inst.zone={} AND inst.version={} "
            "LEFT JOIN dodh_longshadow_turnins settled ON settled.shared_task_id=task.id "
            " AND settled.activity_id=activity.activity_id "
            "WHERE COALESCE(source.inventory_guid,source.item_guid)={} AND source.character_id={} AND source.shared_task_id={} "
            "AND source.accepted_time={} AND UNIX_TIMESTAMP(task.accepted_time)=source.accepted_time "
            "AND source.task_id={} AND source.instance_id={} AND source.item_id={} AND source.source_kind='{}' "
            "AND task.completion_time IS NULL AND (task.expire_time IS NULL OR task.expire_time>NOW()) "
            "AND dz.type=4 "
            "AND activity.done_count=0 AND activity.completed_time IS NULL "
            "AND settled.shared_task_id IS NULL FOR UPDATE",
            route.activity, placement->zone, placement->version, receipt.guid, receipt.character, receipt.occurrence, receipt.accepted_time,
            receipt.task, receipt.mission_instance, receipt.item, kind), false);
        if (!query.Success() || query.RowCount() > 1) return std::nullopt;
        if (query.RowCount() == 1) {
            const auto original_guid = Strings::ToUnsignedInt(query.begin()[0]);
            if (!original_guid) return std::nullopt;
            return LockedDebit{receipt, *debit, &db, original_guid};
        }
    }
    return std::nullopt;
}

// Allocate the durable source key in the same inventory transaction.
// The live serial can be reused after a restart and is only a physical binding.
inline bool RecordSource(ZoneDatabase& db, const Policy::SourceReceipt& source)
{
    const auto* kind = SourceKind(source.source);
    const auto exact = std::any_of(Policy::NormalRoutes.begin(), Policy::NormalRoutes.end(),
        [&](const Policy::Route& route) {
            return route.task == source.task && route.item == source.item && route.source == source.source;
        });
    const auto exact_source = std::any_of(Policy::Sources.begin(), Policy::Sources.end(),
        [&](const Policy::SourceRoute& route) {
            return route.task == source.task && route.item == source.item && route.source == source.source;
        });
    if (!kind || (!exact && !exact_source) || !source.occurrence || !source.accepted_time || !source.character || !source.task ||
        !source.mission_instance || !source.item || source.guid <= 0 || !WriterTransaction(db)) return false;
    const auto result = db.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_item_sources "
        "(inventory_guid,character_id,shared_task_id,accepted_time,task_id,instance_id,item_id,source_kind) "
        "VALUES ({},{},{},{},{},{},{},'{}')", source.guid, source.character, source.occurrence,
        source.accepted_time, source.task, source.mission_instance, source.item, kind), false);
    return result.Success() && result.RowsAffected() == 1;
}

inline bool LockSceneFixture(ZoneDatabase& db, const Policy::Route& route)
{
    const auto actor = Policy::SceneActor(route.effect);
    if (!actor || !WriterTransaction(db)) return false;
    auto engines = db.QueryDatabase(std::string{
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND engine='InnoDB' AND table_name IN ('dodh_longshadow_scene_fixtures',"
        "'dodh_longshadow_scene_effects','dodh_longshadow_turnins','dodh_longshadow_item_sources')"}, false);
    if (!engines.Success() || engines.RowCount() != 1 || !engines.begin()[0] ||
        std::string_view(engines.begin()[0]) != "4") return false;
    const auto fixture = db.QueryDatabase(fmt::format(
        "SELECT f.actor_type FROM dodh_longshadow_scene_fixtures f "
        "JOIN npc_types n ON n.id=f.actor_type "
        "WHERE f.actor_type={} AND f.zone_id={} AND f.zone_version={} AND "
        "((f.actor_type=354920 AND f.loottable_id=0 AND EXISTS "
        " (SELECT 1 FROM npc_types a WHERE a.id=354921)) OR "
        " (f.actor_type=363035 AND n.class=62 AND n.loottable_id=0 AND f.loottable_id=304070103 AND EXISTS "
        " (SELECT 1 FROM loottable l WHERE l.id=f.loottable_id) AND NOT EXISTS "
        " (SELECT 1 FROM spawn2 s WHERE s.id=260808))) FOR UPDATE",
        actor, route.zone, route.version), false);
    return fixture.Success() && fixture.RowCount() == 1;
}

inline bool RecordSettlement(
    ZoneDatabase& db, const Policy::Route& route, const LockedDebit& locked, std::string_view request)
{
    if (!KnownRequestKey(request) || locked.connection != &db || locked.source.task != route.task ||
        locked.source.source != route.source || !WriterTransaction(db)) return false;
    const auto result = db.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_turnins "
        "(shared_task_id,task_id,activity_id,character_id,item_guid,request_key) "
        "VALUES ({},{},{},{},{},'{}')", locked.source.occurrence, route.task, route.activity,
        locked.source.character, locked.original_guid, request), false);
    if (!result.Success() || result.RowsAffected() != 1) return false;
    const auto actor = Policy::SceneActor(route.effect);
    if (!actor) return true;
    const auto reservation = db.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_scene_effects "
        "(request_key,shared_task_id,task_id,activity_id,source_item_guid,character_id,instance_id,actor_type) "
        "VALUES ('{}',{},{},{},{},{},{},{})", request, locked.source.occurrence, route.task, route.activity,
        locked.original_guid, locked.source.character, locked.source.mission_instance, actor), false);
    return reservation.Success() && reservation.RowsAffected() == 1;
}

// No World activity is changed here. The replay worker must acknowledge the
// real effect and the World-committed counter before clearing this row.
inline auto ReadPendingEffects(ZoneDatabase& db, uint64_t occurrence)
{
    return db.QueryDatabase(fmt::format(
        "SELECT t.task_id,t.activity_id,t.character_id,t.item_guid,t.request_key,s.instance_id "
        "FROM dodh_longshadow_turnins t JOIN dodh_longshadow_item_sources s ON s.item_guid=t.item_guid "
        "JOIN kithicor_poison_custody w ON w.request_key=t.request_key AND w.character_id=t.character_id "
        "AND w.shared_task_id=t.shared_task_id AND w.physical=1 AND w.abort_only=0 "
        "WHERE t.shared_task_id={} AND t.effect_applied=0 ORDER BY t.activity_id", occurrence), false);
}
} // namespace DodhMissionCustody
