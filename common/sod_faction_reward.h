#pragma once
#include "common/sod_progression.h"
#include <cstdint>

namespace SodFactionReward {
// GrantSodFaction first verifies an owned selectable reward and a supported
// frozen SoD quote. Only the approved task/faction/base amount is admitted;
// native faction modifiers, caps and atomic receipt persistence stay intact.
constexpr std::uint32_t ExpectedBase(std::uint32_t task, std::uint32_t faction)
{
    if (SodProgression::IsFieldOfScaleTask(task)) {
        if (faction != 1224) return 0u;
        switch (task) {
        case 920199: return 50u;
        case 920198: case 920204: case 920205: case 920206:
        case 920207: case 920208: return 10u;
        case 920197: case 920200: case 920201: case 920202:
        case 920203: case 920209: case 920210: return 20u;
        case 920211: case 920212: case 920213: case 920214:
        case 920215: return 25u;
        default: return 0u;
        }
    }
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
