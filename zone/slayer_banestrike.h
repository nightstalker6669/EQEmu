#pragma once

#include <cstdint>

namespace SlayerBanestrike
{
inline constexpr uint32_t Ability = 15073;
inline constexpr uint32_t HastenedAbility = 15074;
inline constexpr uint32_t Launcher = 37668;
inline constexpr uint32_t SpiritNpcType = 304099980;
inline constexpr uint32_t SlayerCategory = 80;
inline constexpr uint32_t ConquestCategory = 82;
inline constexpr uint32_t SpecialCategory = 83;
inline constexpr uint32_t SkillCategory = 84;

inline constexpr uint8_t WriteBit(uint32_t first_rank)
{
	return first_rank == Ability ? 1 : first_rank == HastenedAbility ? 2 : 0;
}

// The archive has 21 damage ranks through level 100. Five-level breakpoints
// reconstruct the intermediate tiers and retain the observed 90s/100 values.
inline constexpr uint16_t DamageSpell(uint8_t level)
{
	const auto capped_level = level > 100 ? 100 : level;
	return Launcher + 1 + capped_level / 5;
}
}
