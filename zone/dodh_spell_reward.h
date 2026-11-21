#pragma once

#include "common/dodh_hard_variant.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace DodhSpellReward {

struct ClassReward {
    uint32_t item_id;
    uint32_t spell_id;
};

inline constexpr std::array<ClassReward, 17> kLevel69ClassRewards{{
    {0, 0},         // Invalid class
    {0, 0},         // Warrior
    {77898, 8006},  // Cleric
    {77912, 8029},  // Paladin
    {77906, 8019},  // Ranger
    {77910, 8025},  // Shadow Knight
    {77901, 8010},  // Druid
    {0, 0},         // Monk
    {77913, 8030},  // Bard
    {0, 0},         // Rogue
    {77905, 8018},  // Shaman
    {77892, 7999},  // Necromancer
    {77925, 8044},  // Wizard
    {77922, 8040},  // Magician
    {77916, 8034},  // Enchanter
    {77927, 8233},  // Beastlord
    {0, 0},         // Berserker
}};

inline constexpr std::array<uint32_t, 4> kLevel69NormalTasks{920228, 920230, 920232, 920234};

inline constexpr std::array<ClassReward, 17> kLevel68ClassRewards{{
    {0, 0},
    {77893, 8000}, // Commanding Voice
    {77897, 8004}, // Death's Regret
    {77911, 8027}, // Last Rites
    {77907, 8020}, // Hail of Arrows
    {77909, 8022}, // Fickle Shadows
    {77900, 8008}, // Skin of the Reptile
    {77895, 8002}, // Fists of Wu
    {77914, 8031}, // Creeping Dreams
    {77894, 8001}, // Thief's Eyes
    {77903, 8015}, // Lingering Sloth
    {77891, 7995}, // Call for Blood
    {77923, 8041}, // Clinging Frost
    {77920, 8038}, // Burning Aura
    {77917, 8035}, // Echoing Madness
    {77908, 8021}, // Bestial Empathy
    {77896, 8003}, // Cry Havoc
}};
inline constexpr std::array<uint32_t, 5> kLevel68NormalTasks{920236, 920237, 920238, 920239, 920240};

inline constexpr std::array<ClassReward, 17> kLevel70ClassRewards{{
    {0, 0},
    {0, 0},
    {77899, 8007}, // Desperate Renewal
    {0, 0},
    {0, 0},
    {0, 0},
    {77902, 8011}, // Dawnstrike
    {0, 0},
    {0, 0},
    {0, 0},
    {77904, 8017}, // Hungry Plague
    {77890, 7994}, // Dread Pyre
    {77924, 8043}, // Ether Flame
    {77919, 8037}, // Raging Servant
    {77915, 8032}, // Mana Flare
    {0, 0},
    {0, 0},
}};
inline constexpr std::array<uint32_t, 5> kLevel70NormalTasks{920241, 920242, 920243, 920244, 920245};

enum class Route : unsigned { Level69 = 1, Level68 = 2, Level70 = 3 };

inline constexpr std::array kHailRoutes{Route::Level69, Route::Level68, Route::Level70};

inline constexpr const char* ClaimTable(Route route)
{
    switch (route) {
    case Route::Level69: return "dodh_level69_claims";
    case Route::Level68: return "dodh_level68_claims";
    case Route::Level70: return "dodh_level70_claims";
    }
    return nullptr;
}

// The caller reads only acknowledged shared occurrence and member archives.
template <std::size_t N, typename HasCommittedMemberCompletion>
ClassReward ResolveArc(
    uint32_t class_id,
    const std::array<ClassReward, 17>& class_rewards,
    const std::array<uint32_t, N>& normal_tasks,
    HasCommittedMemberCompletion has_completion)
{
    if (class_id >= class_rewards.size() || class_rewards[class_id].item_id == 0) {
        return {};
    }
    for (const auto task_id : normal_tasks) {
        if (has_completion(task_id)) {
            continue;
        }
        const auto hard_task = DodhHardVariant::HardTask(task_id);
        if (!hard_task || !has_completion(hard_task)) {
            return {};
        }
    }
    return class_rewards[class_id];
}

template <typename HasCommittedMemberCompletion>
ClassReward Resolve(Route route, uint32_t class_id, HasCommittedMemberCompletion has_completion)
{
    switch (route) {
    case Route::Level69:
        return ResolveArc(class_id, kLevel69ClassRewards, kLevel69NormalTasks, has_completion);
    case Route::Level68:
        return ResolveArc(class_id, kLevel68ClassRewards, kLevel68NormalTasks, has_completion);
    case Route::Level70:
        return ResolveArc(class_id, kLevel70ClassRewards, kLevel70NormalTasks, has_completion);
    }
    return {};
}

} // namespace DodhSpellReward
