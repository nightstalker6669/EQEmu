/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "common/struct_strategy.h"
#include "common/patches/rof.h"
#include "rof2_limits.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

class EQStreamIdentifier;

namespace EQ { class ItemInstance; }

struct Spawn_Struct;
struct PlayerProfile_Struct;

namespace RoF2 {
 std::optional<std::vector<uint8_t>> SerializeShroudSpawn(const Spawn_Struct&);
 std::optional<std::vector<uint8_t>> SerializeShroudNormalProfile(const PlayerProfile_Struct&);
 std::optional<std::vector<uint8_t>> SerializeShroudInventory(const std::vector<std::pair<int16_t,const EQ::ItemInstance*>>&);
 // Shroud inspection cache entry: native ItemPtr without an ItemPacket type
 // or inventory count. This service has no housing rule dependency.
 std::optional<std::vector<uint8_t>> SerializeShroudPreviewItem(const EQ::ItemInstance &instance);
 // Fully encoded normal ItemPacketTrade for a prepared recovery-bank tree.
 std::optional<std::vector<uint8_t>> SerializeShroudBankItem(const EQ::ItemInstance&,int16_t server_slot);

	// Native embedded ItemPtr, without the OP_ItemPacket type DWORD. Uses the
	// ordinary item encoder and preserves instance identity, charges and children.
	// Housing must be enabled. Full inventory trees and compact estate list
	// ItemPtrs have distinct bounds; the latter retain their existing16-bit frame.
	inline constexpr size_t HousingItemMaximumBytes = 65535;
	inline constexpr size_t HousingItemMaximumNodes = (1 + invbag::SLOT_COUNT) * 7;
	inline constexpr size_t HousingItemMaximumDepth = 8;
	inline constexpr size_t HousingItemMaximumInventoryBytes = 16 * 1024 * 1024;
	std::optional<std::vector<uint8_t>> SerializeHousingItem(
		const EQ::ItemInstance &instance, int16_t server_slot);
	// Bag contents remain exclusively in the canonical custody tree and ordinary
	// inventory delivery. The stored-list bag root keeps its identity/definition.
	std::optional<std::vector<uint8_t>> SerializeHousingItemListEntry(
		const EQ::ItemInstance &instance, int16_t server_slot);


extern void Register(EQStreamIdentifier& into);
extern void Reload();

class Strategy : public StructStrategy
{
public:
	Strategy();

protected:
	virtual std::string Describe() const;
	virtual const EQ::versions::ClientVersion ClientVersion() const;

	//magic macro to declare our opcode processors
#include "ss_declare.h"
#include "rof2_ops.h"
};

}; /*RoF2*/
