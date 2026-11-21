#pragma once

#include "original_trade_recipe.h"
#include <algorithm>
#include <array>
#include <optional>

namespace EQEmu::SharedCore::DodhSabotageCustody {
namespace Trade = OriginalTradeRecipe;

inline constexpr uint32_t Item = 84058;
inline constexpr std::array<uint32_t, 4> NormalSpawns{262525, 262527, 262529, 262531};

struct Deposit {
    uint32_t ordinal;
    Trade::DebitPlan debit;
};

inline std::optional<Deposit> PlanNormal(
    uint32_t task, uint32_t instance, bool activity_active,
    uint32_t zone, uint32_t version, uint32_t npc, uint32_t spawn, const Trade::Offers& offers)
{
    if (task != 920247 || !instance || !activity_active || zone != 354 || version != 3 || npc != 354234) {
        return std::nullopt;
    }
    const auto altar = std::find(NormalSpawns.begin(), NormalSpawns.end(), spawn);
    if (altar == NormalSpawns.end()) {
        return std::nullopt;
    }
    Trade::DebitPlan debit;
    if (!Trade::FirstUnit(offers, Item, debit)) {
        return std::nullopt;
    }
    return Deposit{static_cast<uint32_t>(altar - NormalSpawns.begin()) + 1, debit};
}
}
