// Documentation of retained EQEmu/RoF2 layouts, not a packet parser or new wire recovery.
#pragma once
#include <cstdint>
namespace rof2_mission_documentation {
#pragma pack(push,1)
struct SharedTaskQuit { std::int32_t field1, field2, field3; };
struct SharedTaskNamedRequest { std::int32_t field1, field2; char player_name[64]; };
struct SharedTaskInvite { std::int32_t unknown00, invite_id; char task_name[64], inviter_name[64]; };
struct SharedTaskInviteResponse { std::int32_t unknown00, invite_id; std::int8_t accepted, padding[3]; };
struct SharedTaskAccept { std::int32_t unknown00, unknown04; std::uint32_t npc_entity_id, task_id; float reward_multiplier; };
struct SharedTaskMemberListPrefix { std::uint32_t unknown00, unknown04, member_count; };
// Follow prefix with member_count records: NUL-terminated name, uint32 monster_mission, uint8 leader.
// Actual retained serializer writes N, despite old common-header comment claiming N-1.
struct SharedTaskMemberChangePrefix { std::uint32_t unknown00, unknown04; std::uint8_t added; };
// Follow with NUL-terminated name; no fixed64-byte tail assumed.
struct RoF2DynamicZoneInfo { std::uint32_t client_id, unknown004, assigned, max_players; char dz_name[128], leader_name[64]; };
// RoF2 omits the newer uint32 dz_type tail from this info packet.
struct DynamicZoneChoosePrefix { std::uint32_t client_id, count; };
struct DynamicZoneChoicePrefix { std::uint16_t zone_id, instance_id; std::uint32_t unknown_id1, dz_type, unknown_id2; };
// Each choice prefix is followed by description NUL string and leader NUL string.
struct RoF2DynamicZoneChooseReply {
 std::uint32_t unknown000,unknown004,unknown008,unknown_id1;
 std::uint16_t zone_id,instance_id;
 std::uint32_t dz_type,unknown_id2,unknown028,unknown032,unknown036,unknown040,unknown044,unknown048;
};
#pragma pack(pop)
static_assert(sizeof(SharedTaskQuit)==12);
static_assert(sizeof(SharedTaskNamedRequest)==72);
static_assert(sizeof(SharedTaskInvite)==136);
static_assert(sizeof(SharedTaskInviteResponse)==12);
static_assert(sizeof(SharedTaskAccept)==20);
static_assert(sizeof(SharedTaskMemberListPrefix)==12);
static_assert(sizeof(SharedTaskMemberChangePrefix)==9);
static_assert(sizeof(RoF2DynamicZoneInfo)==208);
static_assert(sizeof(DynamicZoneChoosePrefix)==8);
static_assert(sizeof(DynamicZoneChoicePrefix)==16);
static_assert(sizeof(RoF2DynamicZoneChooseReply)==52);
} // namespace rof2_mission_documentation
