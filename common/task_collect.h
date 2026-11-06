#pragma once

#include <algorithm>
#include <cstdint>

namespace Tasks {

// Collect is a high-water mark of physical custody, not a repeated loot event.
// Rechecking the same stack must never generate additional credit.
inline int CollectInventoryIncrement(int done, int goal, uint64_t carried)
{
	if (done < 0 || goal <= 0 || done >= goal) {
		return 0;
	}
	const auto observed = static_cast<int>(
		std::min(carried, static_cast<uint64_t>(goal))
	);
	return std::max(0, observed - done);
}

} // namespace Tasks
