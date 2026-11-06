// Existing RoF2 layouts from EQEmu common/patches/rof2_structs.h.
// These declarations add no opcode or active mapping. Source protocol facts,
// corroborated for request size by static eqgame.exe analysis, not a packet capture.
#pragma once
#include <cstdint>
#pragma pack(push,1)
struct RoF2InventorySlot {
 std::int16_t type,unknown02,slot,sub_index,aug_index,unknown01;
};
struct RoF2ItemVerifyRequest { RoF2InventorySlot inventory_slot; std::uint32_t target; };
struct RoF2ItemVerifyReply { RoF2InventorySlot inventory_slot; std::uint32_t spell,target; };
#pragma pack(pop)
static_assert(sizeof(RoF2InventorySlot)==12);
static_assert(sizeof(RoF2ItemVerifyRequest)==16);
static_assert(sizeof(RoF2ItemVerifyReply)==20);
