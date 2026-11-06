#pragma once
#include "common/classes.h"
#include "common/skills.h"
#include <cstdint>

// Exact822 source-qualified RRC275/276/277/299 IDs only; no global research policy.
namespace ResearchTablet335 {
inline constexpr bool Selected(std::uint32_t recipe_id) {
 return recipe_id == 3102751 || (recipe_id >= 3100656 && recipe_id <= 3101476);
}
inline constexpr std::uint32_t RequiredKit(std::uint32_t recipe_id) {
 if (!Selected(recipe_id)) return 0;
 if (recipe_id == 3102751) return 41555; // one old Fine Runic Grimoire tome, pure melee
 if (recipe_id <= 3101375) return 93492; // previous715 plus5 Numen chemistry, INT
 if (recipe_id <= 3101474) return 41555; // new99 tomes/media, pure melee
 return 41632; // new2 hybrid spells; other historical paths unqualified
}
inline bool Allows(std::uint32_t recipe_id, std::uint16_t skill, std::uint8_t maker_class,
 std::uint32_t kit_id, std::uint8_t bag_type, std::uint8_t bag_slots, std::uint8_t bag_size) {
 if (!Selected(recipe_id)) return true;
 const auto required_kit = RequiredKit(recipe_id);
 const bool int_caster = maker_class == Class::Necromancer || maker_class == Class::Wizard ||
  maker_class == Class::Magician || maker_class == Class::Enchanter;
 const bool pure_melee = maker_class == Class::Warrior || maker_class == Class::Monk ||
  maker_class == Class::Rogue || maker_class == Class::Berserker;
 const bool hybrid = maker_class == Class::Paladin || maker_class == Class::Ranger ||
  maker_class == Class::ShadowKnight || maker_class == Class::Beastlord;
 const bool allowed_maker = required_kit == 93492 ? int_caster :
  required_kit == 41555 ? pure_melee : required_kit == 41632 && hybrid;
 return allowed_maker && skill == EQ::skills::SkillResearch && kit_id == required_kit &&
  bag_type == 11 && bag_slots == 10 && bag_size == 3;
}
} // namespace ResearchTablet335
