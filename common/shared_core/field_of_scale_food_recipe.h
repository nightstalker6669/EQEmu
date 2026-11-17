#pragma once
#include "weighted_personal_delivery.h"
// Semantic constants from WEIGHTED-TURNIN-CONTRACT.json. This header registers
// no hook and grants no task credit. Unbound values keep Prepare fail-closed.
namespace EQEmu { namespace SharedCore { namespace FieldOfScaleFoodRecipe {
inline WeightedPersonalDelivery::Parameters Unbound() {
 WeightedPersonalDelivery::Parameters p;
 p.task_id=920198; p.activity_id=1; p.pounds_per_unit=10; p.goal_units=10;
 p.roles={{{0,7},{0,4},{0,2}}}; // Chokidai / Scorpion / Snake, IDs pending owner.
 return p;
}
}}}
