#pragma once
#include "common/sod_progression.h"
#include <cstdint>

namespace SodFactionReward {
// GrantSodFaction first verifies an owned selectable reward and a supported
// frozen SoD quote. Only the approved task/faction/base amount is admitted;
// native faction modifiers, caps and atomic receipt persistence stay intact.
constexpr std::uint32_t ExpectedBase(std::uint32_t task, std::uint32_t faction)
{
    if (SodProgression::IsKithicorTask(task)) {
        return faction == 1223 ? (task == 920191 ? 100u : 30u) : 0u;
    }
    const bool sergeant = task == 920175 || task == 920176 || task == 920177;
    const bool hiran = task == 920178 || task == 920179;
    return faction == 1222 ? (sergeant ? 10u : 30u) :
        ((sergeant || hiran) && faction == 1200 ? (sergeant ? 10u : 30u) :
        ((sergeant || hiran) && (faction == 1199 || faction == 1221) ? (sergeant ? 1u : 3u) : 0u));
}
}
