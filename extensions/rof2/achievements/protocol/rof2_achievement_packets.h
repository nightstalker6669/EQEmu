#pragma once
#include <cstdint>
// Documentary fixed-prefix layouts for RoF2 May 10, 2013; little endian.
// Decode fields explicitly on other architectures. Variable records require
// definition-aware serializers; std::string/vector object memory is NOT wire data.
namespace rof2_achievements {
enum class Status : std::int16_t { Completed=0, Open=1, Locked=2, Hidden=3 };
enum class RewardAction : std::uint32_t {
    List=0, Inspect=1, Claim=3, TaskView=4, AchievementView=5, Pending=6, Bulk=7
};
#pragma pack(push, 1)
struct DefinitionsPrefix { std::uint32_t inflated_bytes; /* zlib stream follows */ };
struct PrimaryUpdatePrefix {
    std::uint32_t serial; std::uint8_t dense; std::uint32_t record_count;
    // dense: definition-order State records; sparse: u32 definition_index + State.
};
struct ProgressRecord {
    std::uint32_t achievement_id, component_id, requirement_id, requirement_type, current_count;
};
struct CompareRequest { std::uint32_t target_spawn_id, requester_spawn_id; };
struct EarnedPrefix {
    std::uint32_t achiever_spawn_id, achievement_id, sound_id;
    // NUL-terminated caret-delimited achievement-link metadata follows.
};
struct RewardSourceRequest { std::uint32_t action, source_id; };
struct RewardInspectRequest {
    std::uint32_t action, reward_set_id, option_id, reward_entry_id, item_id;
};
struct RewardClaimRequest {
    std::uint32_t action, pending_reward_id, reward_set_id, selected_wire_option_id, unknown;
};
struct RewardClaimReply {
    std::uint32_t action, pending_reward_id, reward_set_id, selected_subset_id;
    std::uint8_t success, zero[3];
};
#pragma pack(pop)
// Variable State: i16 status; ceil(n1/16) u16 words, then type2, then type0;
// u32 completion timestamp only when completed. Comparison additionally appends
// u32 counts for every component in the same 1,2,0 order. No type3 state array.
// Comparison: zstr player; u32 achievement_id; State(with counts); u8 trailing_flag.
// Progress packet: u32 record_count followed by ProgressRecord[record_count].
constexpr std::uint32_t CommonOnlySelection=0xffffffffu;
} // namespace rof2_achievements
