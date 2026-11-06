#pragma once
#include <cstdint>
#include <cmath>
#include <cstring>
// Original fourteen, two never-learn307 routes, one Northman309 route. No global policy.
namespace FinalRecipe300 {
inline bool Selected(uint32_t id) { return (id >= 3102753 && id <= 3102766) || id == 3102867 || id == 3102868 || id == 3102882; }
inline bool NeverLearn(uint32_t id) { return id == 3102756 || id == 3102757 || id == 3102867 || id == 3102868; }
inline bool Font(uint32_t id) { return id >= 3102758 && id <= 3102764; }
inline uint32_t Skill(uint32_t id) {
 if (id == 3102867) return 64;
 if (id == 3102868 || id == 3102882) return 63;
 if (id <= 3102754) return 69;
 if (id == 3102755) return 63;
 if (NeverLearn(id) || id == 3102766) return 65;
 return 68;
}
inline bool RaceAllowed(uint32_t id, uint32_t race) {
 if (id == 3102882) return race == 2;
 if (id == 3102867 || id == 3102868) return true;
 return id <= 3102754 ? race == 130 : id != 3102755 || race == 9;
}
inline bool WorldAllowed(uint32_t id, uint32_t zone, uint16_t version,
 uint32_t object, uint32_t type, uint32_t item) {
 if (!Selected(id) || Font(id) || item != 0) return false;
 if (id == 3102867) return false; // actual owned portable Fletching Kit only
 if (id == 3102868) return type == 17;
 if (id == 3102882) return zone == 29 && version == 0 && object == 40 && type == 36;
 if (id <= 3102754) return type == 22;
 if (id == 3102755) return zone == 52 && version == 0 && object == 248 && type == 47;
 if (NeverLearn(id)) return type == 19;
 if (id == 3102765) return zone == 490 && version == 0 && object == 228283 && type == 20;
 return id == 3102766 && zone == 202 && version == 0 &&
  (object == 103180 || object == 103189) && type == 19;
}
struct FontPosition { uint8_t door; const char* model; float x,y,z; };
inline FontPosition Position(uint32_t id) {
 switch(id) {
 case 3102758: return {3,"OBJ_FONT_WHITE",45.511f,828.281f,31.9959f};
 case 3102759: return {13,"OBJ_FONT_BLUE",30.745f,823.483f,31.9983f};
 case 3102760: return {14,"OBJ_FONT_PURPLE",15.495f,820.574f,31.9992f};
 case 3102761: return {15,"OBJ_FONT_ORANGE",0.0f,819.599f,31.9987f};
 case 3102762: return {16,"OBJ_FONT_YELLOW",-15.495f,820.574f,31.9989f};
 case 3102763: return {17,"OBJ_FONT_GREEN",-30.745f,823.483f,31.9989f};
 case 3102764: return {18,"OBJ_FONT_BLACK",-45.511f,828.281f,31.9986f};
 default: return {0,"",0,0,0};
 }
}
inline bool InsideFont(const FontPosition& p, const char* model, float dx,float dy,float dz,
 float x,float y,float z) {
 // Source requires INSIDE the corresponding font. Local bounded 5H/8V policy,
 // not an authenticated retail radius. Reject non-finite actor and door coordinates.
 return p.door && model && std::strcmp(p.model,model)==0 &&
  std::isfinite(dx) && std::isfinite(dy) && std::isfinite(dz) &&
  std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
  std::fabs(dx-p.x)<=0.01f && std::fabs(dy-p.y)<=0.01f && std::fabs(dz-p.z)<=0.01f &&
  (x-p.x)*(x-p.x)+(y-p.y)*(y-p.y)<=25.0f && std::fabs(z-p.z)<=8.0f;
}
inline int SummonCount(uint32_t recipe,uint32_t output,int count) {
 // Eight charges on ONE physical cask. Task/madecount/role count remains one.
 return recipe == 3102766 && output == 75815 && count == 1 ? 8 : count;
}
} // namespace FinalRecipe300
