#pragma once
#include "common/serialize_buffer.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace EQ::RewardSelection {
// RoF2 reward-window entry types. RewardSelection::RewardType describes delivery.
enum class RewardWireType : uint32_t {
	Text                         = 0,
	Money                        = 1,
	Item                         = 2,
	Experience                   = 3,
	AlternateAdvancementAbility = 4,
	AlternateAdvancementPoints  = 5,
	GenericPoints                = 11
};

struct RewardDisplayItem {
	uint32_t    field0 = 0;
	uint32_t    field1 = 0;
	uint8_t     field2 = 0;
	uint32_t    field3 = 0;
	std::string text;
};

struct RewardDisplayEntry {
	RewardWireType                 wire_type = RewardWireType::Text;
	std::array<uint32_t, 2>        fields{};
	std::string                    description;
	std::array<uint32_t, 4>        values{};
	uint8_t                        flag = 0;
	std::vector<RewardDisplayItem> items;
};

struct RewardDisplaySubset {
	uint32_t                        subset_id = 0;
	bool                            common_to_all = false;
	uint8_t                         flags = 0;
	std::string                     option_label;
	std::vector<RewardDisplayEntry> entries;
};

struct RewardDisplaySet {
	uint32_t                         pending_reward_id = 0;
	uint32_t                         reward_set_id = 0;
	std::string                      title;
	// IEEE-754 1.0f. The client multiplies numeric reward displays by this.
	uint32_t                         amount_multiplier_bits = 0x3f800000;
	std::vector<RewardDisplaySubset> subsets;
};

// RoF2 opcode 0x6411, action 0. A null display set emits the client-supported
// "no reward" response (action followed by has_reward=0).
SerializeBuffer SerializeRewardDisplay(const RewardDisplaySet *reward_set);

// The bulk action replaces the reward manager with one tab per display set.
// An empty collection clears the manager.
SerializeBuffer SerializeRewardDisplays(
	const std::vector<RewardDisplaySet> &reward_sets
);

SerializeBuffer SerializeRewardDisplayClear();

// RoF2 opcode 0x6411, action 3. The three request identity fields are echoed
// and the final byte tells the client whether to remove the pending reward.
SerializeBuffer SerializeRewardClaimReply(
	uint32_t pending_reward_id,
	uint32_t reward_set_id,
	uint32_t selected_subset_id,
	bool success
);

}
