#pragma once

#include "original_trade_recipe.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace EQEmu::SharedCore::PorChimeExchange {
inline constexpr uint32_t ShrineNpc = 369067;
inline constexpr uint32_t ShrineSpawn = 103627;
inline constexpr char VialExchange = '6';
inline constexpr char FettersExchange = '7';
struct Route {
    uint32_t zone, npc, spawn, input, output, prerequisite;
    char code;
    uint32_t companion = 0;
};
inline constexpr std::array Routes{
    Route{369, 369011, 103459, 84159, 84160, 920336, '1'},
    Route{372, 372000, 103726, 84160, 84161, 920306, '2'},
    Route{369, 369011, 103459, 84161, 84168, 920306, '3'},
    Route{392, 392088, 105896, 84168, 84162, 920309, '4'},
    Route{369, ShrineNpc, ShrineSpawn, 84165, 84166, 0, '5'},
    Route{369, ShrineNpc, ShrineSpawn, 84166, 84167, 0, VialExchange, 84242},
    Route{369, ShrineNpc, ShrineSpawn, 84167, 52601, 0, FettersExchange},
};
inline constexpr std::string_view Prefix{"POR\x01", 4};

inline std::string Encode(char code)
{
    std::string context(Prefix);
    context.push_back(code);
    return context;
}

inline std::string Encode(const Route& route)
{
    return Encode(route.code);
}

inline const Route* Decode(std::string_view context)
{
    if (context.size() != Prefix.size() + 1 || context.substr(0, Prefix.size()) != Prefix) return nullptr;
    for (const auto& route : Routes) {
        if (route.code == context.back()) return &route;
    }
    return nullptr;
}

inline bool Matches(const Route& route, uint64_t zone, uint64_t instance, uint64_t npc, uint64_t spawn)
{
    return instance == 0 && route.zone == zone && route.npc == npc && route.spawn == spawn;
}

inline bool IsInput(const Route& route, uint32_t item)
{
    return item && (item == route.input || item == route.companion);
}

inline bool Plan(const Route& route, const OriginalTradeRecipe::Offers& offers,
                 OriginalTradeRecipe::DebitPlan& debit)
{
    using namespace OriginalTradeRecipe;
    debit = {};
    DebitPlan candidate;
    if (!FirstUnit(offers, route.input, candidate)) return false;
    if (route.npc == ShrineNpc) {
        // The Shrine performs one step per Give. Return mixed stage items
        // intact and require distinct, nonstackable native inventory roots.
        for (const auto& offer : offers) {
            if (!offer.present) continue;
            const bool shrine_item = std::any_of(Routes.begin(), Routes.end(), [&](const auto& other) {
                return other.npc == route.npc && IsInput(other, offer.item_id);
            });
            if (shrine_item && (!IsInput(route, offer.item_id) || offer.stackable)) return false;
        }
    }
    if (route.companion) {
        DebitPlan companion;
        if (!FirstUnit(offers, route.companion, companion)) return false;
        for (std::size_t i = 0; i < candidate.units.size(); ++i) candidate.units[i] += companion.units[i];
    }
    if (!Valid(offers, candidate)) return false;
    debit = candidate;
    return true;
}
}
