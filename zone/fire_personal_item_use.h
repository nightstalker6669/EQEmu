#pragma once
#include "fire_personal_occurrence.h"
#include "common/item_instance.h"
// REVIEW-ONLY, feature-private Fire original item-use context. No native checkout includes this file.
// Owner must include the existing ItemInstance declaration and private occurrence proposal in its native unit.
namespace FirePersonalOccurrenceSource {
struct OriginalItemUse {
 Context personal;
 uint32_t item_id;
 int32_t item_serial;
 int16_t slot;
 int16_t charges_before;
 uint32_t caster_entity;
 uint32_t requested_target_entity;
 uint32_t actual_zone;
 uint32_t actual_instance;
};
// Call only from actual native original-item path before CastSpell/instant completion can run.
// native_slot_supported is the result of the EXISTING native supported-slot/item lookup, not a Lua assertion.
inline std::optional<OriginalItemUse> CaptureOriginalItemUse(const Context& personal,
 const EQ::ItemInstance& original,int16_t actual_slot,bool native_slot_supported,
 uint32_t actual_caster_entity,uint32_t actual_requested_target,
 uint32_t actual_zone,uint32_t actual_instance) {
 const bool combine=personal.consumer==Consumer::CombineIngredients;
 const bool activate=personal.consumer==Consumer::ActivateConstruct;
 if ((!combine && !activate) || personal.activity!=2 || !native_slot_supported ||
  !actual_caster_entity || !original.GetItem() || original.GetSerialNumber()==0) return std::nullopt;
 const auto expected=combine ? personal.rule.ingredient_a : personal.rule.inactive_item;
 if (original.GetID()!=expected || original.GetItem()->Click.Effect!=7651 ||
  original.GetItem()->Click.Type!=3) return std::nullopt;
 // Ingredients B do not click: original cores/essence/orbs are stackable native -1 effect inputs.
 if (activate && (actual_zone!=personal.rule.encounter_zone || actual_instance!=0)) return std::nullopt;
 return OriginalItemUse{personal,original.GetID(),original.GetSerialNumber(),actual_slot,
  original.GetCharges(),actual_caster_entity,actual_requested_target,actual_zone,actual_instance};
}
// Pre-consumption in-process identity check ONLY. No durable GUID or post-consumption receipt is claimed.
inline bool SameOriginalBeforeConsumption(const OriginalItemUse& use,
 const EQ::ItemInstance& current,int16_t current_slot) {
 return current_slot==use.slot && current.GetID()==use.item_id &&
  current.GetSerialNumber()==use.item_serial && current.GetCharges()==use.charges_before;
}
} // namespace FirePersonalOccurrenceSource
