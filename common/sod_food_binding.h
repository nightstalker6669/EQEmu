#pragma once
#include "common/shared_core/field_of_scale_food_recipe.h"
// Exact NPC/item/policy owner bindings must replace this disabled composition
// before activation. Zero is UNBOUND, never a wildcard or historical number.
namespace SodFoodBinding {
inline constexpr bool Enabled=true; // Explicit FoS proposal approved; runtime rule still gates use.
inline constexpr uint32_t Giver=452108; // Existing Severilous; source/NPC addendum pinned.
inline constexpr uint32_t Model=1; // Proposed FoS numeric policy; approved FoS policy.
inline EQEmu::SharedCore::WeightedPersonalDelivery::Parameters Parameters() {
 auto p=EQEmu::SharedCore::FieldOfScaleFoodRecipe::Unbound();
 p.roles={{{88307,7},{88308,4},{88309,2}}}; // Retained actual items; original-period weights.
 // May2010 source reports stacked carcasses returned; retained rows are
 // stackable20, which is not permission to accept a stack in this quest.
 p.stack_rule=EQEmu::SharedCore::WeightedPersonalDelivery::StackRule::ReturnStacked;
 // Explicit LOCAL candidate, not recovered original timing/overage arithmetic.
 p.carry_lifetime_seconds=180;
 p.carry_clock=EQEmu::SharedCore::WeightedPersonalDelivery::CarryClock::RefreshOnAccept;
 p.excess_rule=EQEmu::SharedCore::WeightedPersonalDelivery::ExcessRule::ConsumeEligibleAndDiscardAtCompletion;
 return p; // Runtime EnableFieldOfScaleSlice remains the activation gate.
}
}
