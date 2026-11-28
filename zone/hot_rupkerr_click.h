#pragma once

#include <cstdint>

namespace HouseOfThuleGroup {

constexpr bool IsRupkerrClick(uint32_t item_id, int32_t spell_id)
{
	return (item_id == 49187 && spell_id == 23039) ||
		(item_id == 49195 && spell_id == 7652);
}

} // namespace HouseOfThuleGroup
