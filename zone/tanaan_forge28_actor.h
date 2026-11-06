#pragma once
#include "common/skills.h"
#include <cstdint>
// RRC291 exact world-only bodies. No other Smithing is gated here.
namespace TanaanForge28 {
struct Requirement {std::uint32_t recipe_id,minimum_slots;};
inline constexpr Requirement Requirements[] = {
 {3102554,6},
 {3102555,8},
 {3102556,8},
 {3102557,9},
 {3102558,7},
 {3102559,8},
 {3102560,8},
 {3102561,8},
 {3102562,9},
 {3102563,8},
 {3102564,9},
 {3102565,7},
 {3102566,8},
 {3102567,8},
 {3102568,8},
 {3102569,9},
 {3102570,8},
 {3102571,9},
 {3102572,7},
 {3102573,8},
 {3102574,8},
 {3102575,8},
 {3102576,9},
 {3102577,8},
 {3102578,9},
 {3102579,7},
 {3102580,8},
 {3102581,8},
};
inline const Requirement* Find(std::uint32_t id) noexcept {
 for(const auto& r:Requirements)if(r.recipe_id==id)return &r;
 return nullptr;
}
inline bool Allows(const Requirement& r,std::uint32_t skill,std::uint32_t zone,
 std::uint32_t version,std::uint32_t object_db_id,std::uint32_t object_type,
 std::uint32_t actual_item_id,std::uint32_t actual_world_slots) noexcept {
 return skill==static_cast<std::uint32_t>(EQ::skills::SkillBlacksmithing)&&
  zone==202&&version==0&&(object_db_id==103175||object_db_id==103185||object_db_id==103186||object_db_id==103190)&&object_type==17&&
  actual_item_id==0&&actual_world_slots>=r.minimum_slots;
}
}
