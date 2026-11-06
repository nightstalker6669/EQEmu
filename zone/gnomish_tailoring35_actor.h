#pragma once
#include "common/races.h"
#include "common/skills.h"
#include <cstdint>

// Source-qualified RRC169 only. Do not infer race from names, kit masks or
// learned recipes. This private allowlist does not gate other Tailoring.
namespace GnomishTailoring35 {
inline bool Selected(uint32_t recipe_id) noexcept {
 switch(recipe_id) {
 case 3095953:
 case 3095954:
 case 3095955:
 case 3095956:
 case 3095957:
 case 3095958:
 case 3095959:
 case 3095960:
 case 3095961:
 case 3095962:
 case 3095963:
 case 3095964:
 case 3095965:
 case 3095966:
 case 3095967:
 case 3095968:
 case 3095969:
 case 3095970:
 case 3095971:
 case 3095972:
 case 3095973:
 case 3095974:
 case 3095975:
 case 3095976:
 case 3095977:
 case 3095978:
 case 3095979:
 case 3095980:
 case 3095981:
 case 3095982:
 case 3095983:
 case 3095984:
 case 3095985:
 case 3095986:
 case 3095987:
  return true;
 default:return false;
 }
}
inline bool Allows(uint32_t recipe_id,uint32_t skill,uint32_t bag_type,
 uint32_t actual_container_item,uint32_t actual_profile_race) noexcept {
 if(!Selected(recipe_id))return true;
 return skill==static_cast<uint32_t>(EQ::skills::SkillTailoring) &&
  bag_type==16 && actual_container_item==92877 &&
  actual_profile_race==Race::Gnome;
}
}
