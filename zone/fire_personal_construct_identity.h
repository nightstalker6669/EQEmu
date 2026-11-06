#pragma once
#include "fire_personal_item_use.h"
#include "zone/npc.h"
// REVIEW-ONLY finite Fire construct identity. No spawn, inventory mutation, task credit or reward.
namespace FirePersonalOccurrenceSource {
struct ConstructIdentity {
 OriginalItemUse original_activation;
 uint32_t entity_id;
 uint32_t npc_type_id;
 uint32_t spawn_point_id;
 uint32_t zone_id;
 uint32_t instance_id;
};
// Only the actual native create operation's returned actor may be passed by the private owner.
// Lookup by name/type, quest history or a later observed actor is not this capture seam.
inline std::optional<ConstructIdentity> CaptureActualReturnedConstruct(
 const OriginalItemUse& original,NPC& actual_returned,uint32_t actual_zone,uint32_t actual_instance) {
 if(original.personal.consumer!=Consumer::ActivateConstruct ||
  original.item_id!=original.personal.rule.inactive_item || !actual_returned.GetID() ||
  actual_returned.GetNPCTypeID()!=original.personal.rule.construct_npc ||
  actual_zone!=original.personal.rule.encounter_zone || actual_instance!=0 ||
  original.actual_zone!=actual_zone || original.actual_instance!=actual_instance)
  return std::nullopt;
 return ConstructIdentity{original,actual_returned.GetID(),actual_returned.GetNPCTypeID(),
  actual_returned.GetSpawnPointID(),actual_zone,actual_instance};
}
// Only an identity predicate inside the actual native death-complete call, not a commit/credit API.
inline bool MatchesActualConstructDeath(const ConstructIdentity& original,NPC& actual_dead,
 uint32_t actual_killed_entity,uint32_t actual_zone,uint32_t actual_instance) {
 return actual_killed_entity!=0 && actual_killed_entity==original.entity_id &&
  actual_dead.GetID()==original.entity_id && actual_dead.GetNPCTypeID()==original.npc_type_id &&
  actual_dead.GetSpawnPointID()==original.spawn_point_id && actual_zone==original.zone_id &&
  actual_instance==original.instance_id;
}
// Caster-only source context. Does NOT select/implement original 920169 eligible-group credit.
inline std::optional<Context> CasterDeathAcceptanceContext(Database& db,
 const ConstructIdentity& original,uint32_t actual_character_id,TaskType actual_type,
 const ClientTaskInformation& actual_personal) {
 return ForConsumer(db,original.original_activation.personal.acceptance,actual_character_id,
  actual_type,actual_personal,Consumer::ActualConstructDeath);
}
} // namespace FirePersonalOccurrenceSource
