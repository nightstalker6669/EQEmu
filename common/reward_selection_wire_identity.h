#pragma once
#include "reward_selection.h"
#include <algorithm>
#include <limits>
#include <optional>

namespace EQ::RewardSelection::WireIdentity {

inline bool SameOption(const Option &a, const Option &b)
{
	return a.option_id == b.option_id && a.sequence == b.sequence && a.label == b.label &&
		a.common_to_all == b.common_to_all && a.flags == b.flags && a.class_mask == b.class_mask &&
		a.rewards.size() == b.rewards.size() && std::equal(a.rewards.begin(), a.rewards.end(), b.rewards.begin(),
			[](const Reward &x, const Reward &y) {
				return x.entry_id == y.entry_id && x.type == y.type && x.data_id == y.data_id &&
					x.amount == y.amount && x.description == y.description;
			});
}

inline bool SameCatalog(const Set &a, const Set &b)
{
	return a.reward_set_id == b.reward_set_id && a.title == b.title && a.options.size() == b.options.size() &&
		std::equal(a.options.begin(), a.options.end(), b.options.begin(), SameOption);
}

inline uint64_t Required(const Set &current, const Set *previous)
{
	uint64_t count = !previous || !SameCatalog(current, *previous) ? 1 : 0;
	for (const auto &option : current.options) {
		if (!previous || std::none_of(previous->options.begin(), previous->options.end(),
			[&](const Option &p) { return p.wire_option_id && SameOption(option, p); })) ++count;
	}
	return count;
}

// Production supplies a globally reserved, committed range. Retired tokens
// cannot bind a later session, including across reconnects and restarts. A
// changed visible catalogue also needs a new RoF2 cache identity.
inline std::optional<uint32_t> Assign(Set &current, const Set *previous, uint32_t previous_wire_set,
	uint64_t &next_set, uint64_t &next_option)
{
	if (previous && !previous_wire_set) return std::nullopt;
	uint32_t wire_set = previous && SameCatalog(current, *previous) ? previous_wire_set : 0;
	if (!wire_set) {
		if (next_set > std::numeric_limits<uint32_t>::max()) return std::nullopt;
		wire_set = static_cast<uint32_t>(next_set++);
	}
	for (auto &option : current.options) {
		option.wire_option_id = 0;
		if (previous) {
			const auto old = std::find_if(previous->options.begin(), previous->options.end(),
				[&](const Option &p) { return SameOption(option, p); });
			if (old != previous->options.end()) option.wire_option_id = old->wire_option_id;
		}
		if (!option.wire_option_id) {
			if (next_option >= CommonOnlySelection) return std::nullopt;
			option.wire_option_id = static_cast<uint32_t>(next_option++);
		}
	}
	return wire_set;
}

} // namespace EQ::RewardSelection::WireIdentity
