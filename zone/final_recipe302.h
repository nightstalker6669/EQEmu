#pragma once
#include <cstdint>
namespace FinalRecipe302 {
struct Requirement{uint32_t recipe,output,skill,kit,type,slots,size,yield;};
inline constexpr Requirement Requirements[]={
 {3102780,12607,65,17987,30,2,1,3},
 {3102781,57550,59,57704,9,6,3,1},
 {3102782,75534,64,18,18,10,4,5},
 {3102783,75535,64,18,18,10,4,5},
 {3102784,75544,64,18,18,10,4,10},
 {3102785,96534,64,18,18,10,4,1},
 {3102786,96943,64,18,18,10,4,1},
 {3102787,98490,59,17901,9,8,3,1},
};
inline const Requirement* Find(uint32_t id){for(const auto& r:Requirements)if(r.recipe==id)return &r;return nullptr;}
}
