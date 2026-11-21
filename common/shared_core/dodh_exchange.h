#pragma once

#include "original_trade_recipe.h"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace EQEmu::SharedCore::DodhExchange {
inline constexpr unsigned JournalKind = 4;
inline constexpr char JournalContext[] = "DODH\x01\x01";
inline constexpr uint32_t Zone = 365;
inline constexpr uint32_t Giver = 365284;
inline constexpr uint32_t Spawn = 103431;
inline constexpr uint32_t Output = 88005;
inline constexpr uint32_t Lens = 88034;
inline constexpr uint32_t Monocle = 88013;
inline constexpr char MonocleContext[] = "DODH\x01\x04";
inline constexpr std::array<uint32_t, 2> MonocleParts{Output, Lens};
inline constexpr uint32_t OpenerNpc = 365275;
inline constexpr uint32_t OpenerSpawn = 103414;
inline constexpr uint32_t Notebook = 84040;
inline constexpr char OpenerContext[] = "DODH\x01\x03";
inline constexpr uint32_t Guard = 86018;
inline constexpr uint32_t DesertingTask = 920224;
inline constexpr uint32_t DesertingZone = 350;
inline constexpr uint32_t DesertingVersion = 1;
inline constexpr std::string_view GuardPrefix{"DODH\x01\x02", 6};

inline bool GuardActor(uint32_t npc, uint32_t spawn)
{
    switch (spawn) {
    case 263220: return npc == 350035;
    case 263223: return npc == 350037;
    case 263226: return npc == 350038;
    case 263221: case 263222: case 263224: case 263225: case 263227:
    case 263228: case 263229: case 263231: case 263232: return npc == 350036;
    default: return false;
    }
}

struct GuardIdentity {
    uint64_t run;
    uint32_t dz;
    uint32_t entity;
    uint32_t generation;
};

inline std::string EncodeGuard(const GuardIdentity& id)
{
    std::string out(GuardPrefix);
    const auto append = [&](uint64_t value, unsigned width) {
        for (unsigned i = 0; i < width; ++i) out += char(uint8_t(value >> (8 * i)));
    };
    append(id.run, 8);
    append(id.dz, 4);
    append(id.entity, 4);
    append(id.generation, 4);
    return out;
}

inline std::optional<GuardIdentity> DecodeGuard(std::string_view context)
{
    if (context.size() != GuardPrefix.size() + 20 || context.substr(0, GuardPrefix.size()) != GuardPrefix)
        return std::nullopt;
    std::size_t offset = GuardPrefix.size();
    const auto read = [&](unsigned width) {
        uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i) value |= uint64_t(uint8_t(context[offset++])) << (8 * i);
        return value;
    };
    GuardIdentity id{read(8), uint32_t(read(4)), uint32_t(read(4)), uint32_t(read(4))};
    if (!id.run || !id.dz || !id.entity || !id.generation) return std::nullopt;
    return id;
}
inline constexpr std::array<uint32_t, 4> Parts{88001, 88010, 88011, 88012};

inline bool IsKind(unsigned kind)
{
    return kind == JournalKind;
}

inline bool IsPart(uint32_t item)
{
    return std::find(Parts.begin(), Parts.end(), item) != Parts.end();
}

template<std::size_t Count>
inline bool PlanParts(const std::array<uint32_t, Count>& parts, const OriginalTradeRecipe::Offers& offers,
                      OriginalTradeRecipe::DebitPlan& debit)
{
    debit = {};
    OriginalTradeRecipe::DebitPlan candidate;
    for (const auto item : parts) {
        if (!item) continue;
        const auto found = std::find_if(offers.begin(), offers.end(), [item](const auto& offer) {
            return offer.present && offer.item_id == item;
        });
        if (found == offers.end()) {
            return false;
        }
        candidate.units[found - offers.begin()] = 1;
    }
    if (!OriginalTradeRecipe::Valid(offers, candidate)) {
        return false;
    }
    debit = candidate;
    return true;
}

inline bool Plan(const OriginalTradeRecipe::Offers& offers, OriginalTradeRecipe::DebitPlan& debit)
{
    return PlanParts(Parts, offers, debit);
}

// Materialize these seven values from the single Database reservation.
inline constexpr uint32_t LibraryTask = 920280;
inline constexpr uint32_t EyesTask = 920281;
inline constexpr uint32_t MemoriesTask = 920282;
inline constexpr uint32_t MistyTask = 920283;
inline constexpr uint32_t EyeBoundTask = 920284;
inline constexpr uint32_t ColdwindNpc = 25920;
inline constexpr uint32_t ColdwindSpawn = 304066072;
inline constexpr uint32_t ColdwindGlobalNpc = 25414;
inline constexpr uint32_t ColdwindGlobalSpawn = 97602;
inline constexpr uint32_t ColdwindGlobalInstance = 1;

struct PublicRoute {
    uint8_t group;
    uint8_t code;
    uint32_t zone;
    uint32_t npc;
    uint32_t spawn;
    uint32_t task;
    uint8_t final_activity;
    std::array<uint32_t, 3> items;
};

inline constexpr PublicRoute PublicRoutes[] = {
    {5, 1, 365, Giver, Spawn, LibraryTask, 1, {88006, 88007, 0}},
    {5, 2, 365, Giver, Spawn, EyesTask, 3, {88017, 88018, 88019}},
    {5, 3, 351, 351108, 101696, MemoriesTask, 2, {88023, 0, 0}},
    {5, 4, 25, ColdwindNpc, ColdwindSpawn, MistyTask, 4, {88022, 88020, 0}},
    {5, 5, 25, ColdwindNpc, ColdwindSpawn, EyeBoundTask, 0, {88035, 88021, 0}},
    {6, 1, 351, 351015, 101429, 0, 0, {52522, 0, 0}},
    {6, 2, 351, 351030, 101475, 0, 0, {52523, 0, 0}},
    {6, 3, 351, 351072, 101584, 0, 0, {52524, 0, 0}},
    {6, 4, 351, 351087, 101623, 0, 0, {52525, 0, 0}},
    {7, 1, 351, 351110, 101704, 505745, 1, {0, 0, 0}},
    {7, 2, 351, 351111, 101706, 505745, 1, {0, 0, 0}},
    {7, 3, 351, 351112, 101707, 505745, 1, {0, 0, 0}},
    {7, 4, 351, 351113, 101708, 505745, 1, {0, 0, 0}},
    {7, 5, 351, 351012, 101410, 505745, 2, {0, 0, 0}},
    {7, 6, 351, 351107, 101695, 505745, 3, {88009, 0, 0}},
};

inline bool RouteReady(const PublicRoute& route)
{
    return route.npc && route.spawn && (route.group == 6 || route.task);
}

inline constexpr bool IsColdwindRoute(const PublicRoute& route)
{
    return route.group == 5 && (route.code == 4 || route.code == 5);
}

inline constexpr bool PublicRouteActor(const PublicRoute& route, uint32_t zone, uint32_t instance,
    uint32_t npc, uint32_t spawn)
{
    if (zone != route.zone) return false;
    if (instance == 0) return npc == route.npc && spawn == route.spawn;
    return IsColdwindRoute(route) && instance == ColdwindGlobalInstance &&
        npc == ColdwindGlobalNpc && spawn == ColdwindGlobalSpawn;
}

inline constexpr bool PublicRouteScene(const PublicRoute& route, uint32_t zone, uint32_t version,
    uint32_t instance, uint32_t npc, uint32_t spawn)
{
    return version == (instance == 0 ? 0u : 1u) && PublicRouteActor(route, zone, instance, npc, spawn);
}

inline std::string RouteContext(const PublicRoute& route, uint32_t accepted = 0)
{
    std::string bytes("DODH\x01", 5);
    bytes.push_back(char(route.group));
    bytes.push_back(char(route.code));
    if (route.group == 7 && accepted) bytes += std::to_string(accepted);
    return bytes;
}

inline const PublicRoute* DecodeRoute(std::string_view bytes)
{
    for (const auto& route : PublicRoutes) {
        if (!RouteReady(route)) continue;
        const auto prefix = RouteContext(route);
        if (route.group != 7 && prefix == bytes) return &route;
        if (route.group == 7 && bytes.size() > prefix.size() && bytes.size() <= prefix.size() + 10 &&
            bytes.substr(0, prefix.size()) == prefix &&
            std::all_of(bytes.begin() + prefix.size(), bytes.end(), [](char c) { return c >= '0' && c <= '9'; }))
            return &route;
    }
    return nullptr;
}

inline bool RoutePart(const PublicRoute& route, uint32_t item)
{
    return item && std::find(route.items.begin(), route.items.end(), item) != route.items.end();
}

inline bool PlanRoute(const PublicRoute& route, const OriginalTradeRecipe::Offers& offers,
                      OriginalTradeRecipe::DebitPlan& debit)
{
    return PlanParts(route.items, offers, debit);
}

inline bool IsMonoclePart(uint32_t item)
{
    return item == Output || item == Lens;
}
}
