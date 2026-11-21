#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace DodhRakbanClaim {

struct Mission {
    uint32_t normal;
    uint32_t eligible_hard = 0;
};

// Exact identities come from the sealed Darkhollow reservation.
// Keep the master flow off until the owning release approves its activation.
// The dated launch-era account makes the Bellfast rescue optional. Keep it
// provisionable as an individual detail and enable it here only with evidence.
inline constexpr uint32_t kFortuneTeller = 362165;
inline constexpr bool kMasterEnabled = true;
inline constexpr uint32_t kFortuneTellerSpawn = 102807;
inline constexpr uint32_t kMasterTask = 920279;
inline constexpr uint32_t kRecipe = 30752007;
inline constexpr bool kRequireBellfastRescue = false;

inline constexpr std::array<Mission, 4> kBellfast{{{920270}, {920271}, {920272}, {920273}}};
inline constexpr Mission kBellfastRescue{920274};
inline constexpr std::array<Mission, 5> kLongshadow{{{920251}, {920252}, {920253}, {920254}, {920255}}};
inline constexpr std::array<Mission, 4> kWiddlethorpe{{{920275}, {920276}, {920277}, {920278}}};
inline constexpr std::array<Mission, 5> kJarzarrad{{
    {920241}, {920242}, {920243}, {920244}, {920245}
}};
inline constexpr std::array<Mission, 4> kBrovil{{
    {920228}, {920230}, {920232}, {920234}
}};

inline constexpr unsigned kShroudClaim = 6;

template <typename HasCompletion, typename AlreadyClaimed>
uint32_t Resolve(unsigned slot, HasCompletion has_completion, AlreadyClaimed already_claimed)
{
    const auto earned = [&](Mission mission) {
        return (mission.normal && has_completion(mission.normal)) ||
               (mission.eligible_hard && has_completion(mission.eligible_hard));
    };
    const auto complete = [&](const auto &arc) {
        return std::all_of(arc.begin(), arc.end(), earned);
    };
    switch (slot) {
    case 1:
        return complete(kBellfast) && (!kRequireBellfastRescue || earned(kBellfastRescue)) ? 85566 : 0;
    case 2: return complete(kLongshadow) ? 85567 : 0;
    case 3: return complete(kWiddlethorpe) ? 85568 : 0;
    case 4: return complete(kJarzarrad) ? 85569 : 0;
    case 5: return complete(kBrovil) ? 85570 : 0;
    case kShroudClaim:
        for (unsigned fragment = 1; fragment < kShroudClaim; ++fragment) {
            if (!already_claimed(fragment)) {
                return 0;
            }
        }
        return 85575;
    default: return 0;
    }
}

} // namespace DodhRakbanClaim
