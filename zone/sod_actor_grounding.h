#pragma once

#include "npc.h"
#include "map.h"
#include <cmath>
#include <optional>

// Approved local physical bindings. The retained values are FLOOR samples,
// never NPC origins. Resolve through the actual loaded NPC's native offsets.
namespace SodActorGrounding {
struct Profile {
 uint32 npc_id;
 uint32 spawn_id;
 float x;
 float y;
 float floor;
};
struct Result { float floor; float offset; float origin; };

inline std::optional<Profile> Binding(uint32 npc_id) {
 switch(npc_id) {
 case 467914: return Profile{467914,304064034,-210.99566650390625f,280.3129577636719f,-72.08487701416016f};
 case 467915: return Profile{467915,304064035,-515.0f,244.0f,-182.02853393554688f};
 default: return std::nullopt;
 }
}

inline std::optional<Result> Resolve(NPC& actor,const Profile& p) {
 if(actor.GetNPCTypeID()!=p.npc_id ||
    std::abs(actor.GetX()-p.x)>0.001f || std::abs(actor.GetY()-p.y)>0.001f) return std::nullopt;
 // GetGroundZ calls the serving map's native FindBestZ. Its third argument
 // is relative to this actor's current Z; probe above the measured floor.
 const float floor=actor.GetGroundZ(p.x,p.y,p.floor+8.0f-actor.GetZ());
 const float offset=actor.GetZOffset();
 const float origin=floor+offset;
 if(floor==BEST_Z_INVALID || !std::isfinite(floor) ||
    std::abs(floor-p.floor)>2.0f || !std::isfinite(offset) || !std::isfinite(origin)) return std::nullopt;
 return Result{floor,offset,origin};
}
}
