#pragma once

#include "common/database.h"
#include "common/dynamic_zone_lockout.h"
#include "common/shared_core/vergalid_mission.h"
#include "common/shared_task_admission.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <vector>

namespace VergalidMission {
using MemberLockouts = std::unordered_map<uint32_t, std::vector<DzLockout>>;

inline std::optional<MemberLockouts> ReadMemberLockouts(Database& db, const std::vector<uint32_t>& members)
{
    if (members.empty() || std::ranges::find(members, 0) != members.end()) return std::nullopt;
    auto rows = db.QueryDatabase(fmt::format(
        "SELECT character_id,event_name,from_expedition_uuid,UNIX_TIMESTAMP(expire_time),duration "
        "FROM character_expedition_lockouts WHERE character_id IN ({}) AND BINARY expedition_name='{}' "
        "AND BINARY event_name IN ('Nurtha Frazzlesprocket','Goru Uldrock','Vergalid') AND expire_time>NOW()",
        fmt::join(members, ","), db.Escape(Name)));
    if (!rows.Success()) return std::nullopt;
    MemberLockouts result;
    for (auto row = rows.begin(); row != rows.end(); ++row) {
        const auto id = SharedTaskAdmission::Number(row[0]);
        const auto until = SharedTaskAdmission::Number(row[3]);
        const auto duration = SharedTaskAdmission::Number(row[4]);
        if (!id || !*id || *id > UINT32_MAX || !row[1] || !FindEvent(row[1]) ||
            !row[2] || std::string_view(row[2]).size() != 36 || !until || !*until ||
            !duration || !*duration || *duration > UINT32_MAX) return std::nullopt;
        auto& owned = result[static_cast<uint32_t>(*id)];
        if (std::ranges::any_of(owned, [&](const auto& lockout) { return lockout.IsEvent(row[1]); }))
            return std::nullopt;
        owned.emplace_back(row[2], Name, row[1], static_cast<time_t>(*until), static_cast<uint32_t>(*duration));
    }
    return result;
}

inline bool Compatible(const std::vector<DzLockout>& zone_lockouts, const std::vector<DzLockout>& member,
    std::string_view current_uuid = {})
{
    return std::ranges::all_of(member, [&](const auto& lockout) {
        const bool final = lockout.IsEvent("Vergalid");
        if (final && (current_uuid.empty() || lockout.UUID() != current_uuid)) return false;
        return std::ranges::any_of(zone_lockouts, [&](const auto& seed) {
            return !seed.IsExpired() && seed.IsEvent(lockout.Event()) &&
                (!final || seed.UUID() == current_uuid);
        });
    });
}

inline std::optional<std::vector<DzLockout>> Inherit(Database& db, uint32_t requester,
    const std::vector<uint32_t>& members)
{
    if (std::ranges::count(members, requester) != 1) return std::nullopt;
    auto locked = ReadMemberLockouts(db, members);
    if (!locked) return std::nullopt;
    auto& leader = (*locked)[requester];
    for (const auto& entry : *locked) {
        // There is no current Mission UUID on a new request, so even the
        // requester cannot inherit the final Vergalid event.
        if (!Compatible(leader, entry.second)) return std::nullopt;
    }
    return leader;
}
}
