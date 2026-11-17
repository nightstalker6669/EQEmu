#pragma once
#include "inventory_session_service.h"
#include <cstdint>
#include <string>
class Database;
namespace WeightedDeliveryRecovery {
inline void ValidatePhysicalOutcome(Database&,const EQEmu::SharedCore::InventorySessions::Lease&,
 const std::string&,uint64_t,const char*);
inline void MarkConfirmedFullReturn(Database&,const EQEmu::SharedCore::InventorySessions::Lease&,
 const std::string&,uint64_t,const char*);
}
// One canonical FSWTv1 context and Spells semantic helper, no copied codec or
// external Zone/Lua linkage. Common supervisor and complete-delete both use it.
#include "../sod_weighted_delivery_recovery_adapter.h"
