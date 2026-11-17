#pragma once
#include "common/shared_core/inventory_session_service.h"
#include "common/sod_weighted_delivery_context.h"
#include "common/sod_weighted_delivery_recovery.h"

// Include after Core's weighted_delivery_recovery.h declarations. Header-only
// common linkage reaches Zone complete-delete AND the supervisor recovery TU.
namespace WeightedDeliveryRecovery {
inline void ValidatePhysicalOutcome(Database& db,
 const EQEmu::SharedCore::InventorySessions::Lease& lease,const std::string& key,
 uint64_t physical,const char* context_hex) {
 const auto bytes=SodWeightedDeliveryContext::Unhex(context_hex);
 const auto c=SodWeightedDeliveryContext::Decode(bytes);
 if(!lease.active || lease.actor.character_id!=c.occurrence.character_id ||
  lease.owner.zone_id!=452 || lease.owner.instance_id || physical>3 || physical==2)
  throw std::runtime_error("Food original recovery lease/domain differs");
 const auto state=SodWeightedDeliveryRecovery::Inspect(db,SodWeightedDeliveryContext::Identity(c),key,bytes);
 if(!SodWeightedDeliveryRecovery::MatchesPhysical(state,static_cast<unsigned>(physical)))
  throw std::runtime_error("Food original physical/semantic outcome differs");
}
inline void MarkConfirmedFullReturn(Database& db,
 const EQEmu::SharedCore::InventorySessions::Lease& lease,const std::string& key,
 uint64_t physical,const char* context_hex) {
 const auto bytes=SodWeightedDeliveryContext::Unhex(context_hex);
 const auto c=SodWeightedDeliveryContext::Decode(bytes);
 if(!lease.active || lease.actor.character_id!=c.occurrence.character_id ||
  lease.owner.zone_id!=452 || lease.owner.instance_id || (physical!=0 && physical!=3) ||
  !SodWeightedDeliveryRecovery::MarkAfterVerifiedFullReturn(db,
   SodWeightedDeliveryContext::Identity(c),key,bytes,static_cast<unsigned>(physical)))
  throw std::runtime_error("Food verified full-return semantic settlement differs");
}
}
