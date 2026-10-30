#pragma once
#include "../repositories/inventory_repository.h"
#include <cstdint>
#include <vector>
namespace EQEmu::SharedCore {
inline constexpr uint8_t NullItemId=1,NullCharges=2,NullCustomData=4,NullGuid=8;
struct InventoryRow {
 InventoryRepository::Inventory values{}; uint8_t null_fields=0;
 std::vector<uint8_t> evolving_payload;
 std::vector<InventoryRow> children;
};
}
// Compatibility names preserve existing feature APIs and serialized receipt bytes.
namespace Housing::LiveItems {
using EQEmu::SharedCore::InventoryRow;
using EQEmu::SharedCore::NullItemId; using EQEmu::SharedCore::NullCharges;
using EQEmu::SharedCore::NullCustomData; using EQEmu::SharedCore::NullGuid;
}
