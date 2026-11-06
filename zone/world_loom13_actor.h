#pragma once
#include "common/skills.h"
#include <cstdint>
// RRC290 exact world-only bodies. No other Tailoring is gated here.
namespace WorldLoom13 {
struct Requirement {std::uint32_t recipe_id,minimum_slots;};
inline constexpr Requirement Requirements[] = {
 {3102522,4},
 {3102523,5},
 {3102524,6},
 {3102525,4},
 {3102526,6},
 {3102527,3},
 {3102528,3},
 {3102529,6},
 {3102530,6},
 {3102531,6},
 {3102532,6},
 {3102533,4},
 {3102534,3},
};
inline const Requirement* Find(std::uint32_t id) noexcept {
 for(const auto& r:Requirements)if(r.recipe_id==id)return &r;
 return nullptr;
}
inline bool Allows(const Requirement& r,std::uint32_t skill,std::uint32_t zone,
 std::uint32_t version,std::uint32_t object_db_id,std::uint32_t object_type,
 std::uint32_t actual_item_id,std::uint32_t actual_world_slots) noexcept {
 return skill==static_cast<std::uint32_t>(EQ::skills::SkillTailoring)&&
  zone==202&&version==0&&object_db_id==228440&&object_type==16&&
  actual_item_id==0&&actual_world_slots>=r.minimum_slots;
}
}
