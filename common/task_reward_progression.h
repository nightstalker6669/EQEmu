#pragma once

#include "reward_selection.h"
#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace Tasks {

struct RewardProgressionPolicy {
	std::vector<uint32_t> qualifying_tasks;
	std::unordered_map<uint32_t, uint32_t> option_minimum_completions;
};

template <typename Completed>
uint32_t DistinctRewardCompletions(const RewardProgressionPolicy &policy,
	Completed completed)
{
	std::unordered_set<uint32_t> counted;
	for (const auto task_id : policy.qualifying_tasks) {
		if (task_id && completed(task_id)) {
			counted.insert(task_id);
		}
	}
	return static_cast<uint32_t>(counted.size());
}

inline std::optional<EQ::RewardSelection::Set> FilterRewardProgression(
	const EQ::RewardSelection::Set &set,
	const RewardProgressionPolicy *policy,
	std::optional<uint32_t> frozen_count)
{
	// A removed policy or missing completion snapshot must not broaden a claim.
	if (bool(policy) != frozen_count.has_value()) {
		return std::nullopt;
	}
	auto result = set;
	if (!policy) {
		return result;
	}
	if (*frozen_count > policy->qualifying_tasks.size()) {
		return std::nullopt;
	}
	std::erase_if(result.options, [&](const auto &option) {
		const auto gate = policy->option_minimum_completions.find(option.option_id);
		return gate != policy->option_minimum_completions.end() &&
			*frozen_count < gate->second;
	});
	return result.options.empty() ? std::nullopt : std::optional(std::move(result));
}

} // namespace Tasks
