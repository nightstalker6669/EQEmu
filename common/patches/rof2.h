/*	EQEMu: Everquest Server Emulator
	
	Copyright (C) 2001-2016 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef COMMON_ROF2_H
#define COMMON_ROF2_H

#include "../struct_strategy.h"
#include "rof2_limits.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace EQ { class ItemInstance; }

class EQStreamIdentifier;

namespace RoF2
{

	//these are the only public member of this namespace.
	extern void Register(EQStreamIdentifier &into);
	extern void Reload();
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



	//you should not directly access anything below..
	//I just dont feel like making a seperate header for it.

	class Strategy : public StructStrategy {
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

#endif /*COMMON_ROF2_H*/
