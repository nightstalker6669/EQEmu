#pragma once

#include "reward_selection.h"
#include <algorithm>
#include <optional>

namespace EQ::SpellTask {

enum class Learning { NotSpell, Unknown, Known, Invalid };

struct Policy {
	uint32_t return_item_id = 0;
	int16_t language_id = -1;
	uint8_t language_skill = 0;
	uint32_t faction_id = 0;
	uint8_t maximum_faction_con = 0;
	bool hide_known_spells = false;
};

// A null snapshot is invalid state, not the legacy unrestricted policy.
// Once selected, an occurrence must retain its exact entries for ledger retries.
template <typename ItemLearning>
std::optional<RewardSelection::Set> Filter(
	const RewardSelection::Set &set, std::optional<bool> hide_known,
	uint16_t base_class, uint32_t locked_option_id, ItemLearning learning)
{
	if (!hide_known) {
		return std::nullopt;
	}
	if (!*hide_known || locked_option_id) {
		return set;
	}
	auto result = set;
	bool invalid = false;
	std::erase_if(result.options, [&](const auto &option) {
		if (!RewardSelection::OptionEligibleForClass(option, base_class)) {
			return true;
		}
		if (option.common_to_all || option.rewards.empty()) {
			return false;
		}
		bool all_known = true;
		for (const auto &reward : option.rewards) {
			if (reward.type != RewardSelection::RewardType::Item) {
				all_known = false;
				continue;
			}
			const auto state = learning(reward.data_id);
			invalid |= state == Learning::Invalid;
			all_known &= state == Learning::Known;
		}
		return all_known;
	});
	return invalid || result.options.empty() ? std::nullopt : std::optional(std::move(result));
}

} // namespace EQ::SpellTask
