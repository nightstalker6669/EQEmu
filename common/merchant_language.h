#pragma once

#include <cstdint>

namespace EQ::MerchantLanguage {

inline constexpr int LanguageCount = 28;
inline constexpr int MaximumSkill = 100;

// -1/0 preserves the existing unrestricted catalogue. Invalid configured
// requirements fail closed, before a language ID can index a player profile.
inline constexpr bool ValidRequirement(int language, int minimum)
{
	return (language == -1 && minimum == 0) ||
		(language >= 0 && language < LanguageCount &&
		 minimum >= 0 && minimum <= MaximumSkill);
}

inline constexpr bool Eligible(int language, int minimum, int learned)
{
	return ValidRequirement(language, minimum) &&
		(minimum == 0 || (learned >= minimum && learned <= MaximumSkill));
}

} // namespace EQ::MerchantLanguage
