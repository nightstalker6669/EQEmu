#ifndef EQEMU_FELLOWSHIP_CAMPSITE_MATERIALS_H
#define EQEMU_FELLOWSHIP_CAMPSITE_MATERIALS_H
#include "campsite_catalogue.h"
#include "../housing/inventory_session_service.h"
#include "../housing/live_item_service.h"
#include <array>
namespace Fellowship { namespace Campsites {
// Requires the caller's existing nonretry transaction. Both inventory debits
// and SetCampfire must commit on this same connection. Does not touch runtime
// inventory; the authenticated zone applies its exact planned delta afterward.
bool ConsumeMaterialsUnderTransaction(Database& database, uint32_t campsite_id,
 const Housing::InventorySessions::Lease& lease,
 const std::array<Housing::LiveItems::InventoryRow, 2>& expected);
} }
#endif
