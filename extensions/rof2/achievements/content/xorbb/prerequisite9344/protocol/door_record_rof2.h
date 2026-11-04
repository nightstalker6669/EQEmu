#pragma once
#include <cstddef>
#include <cstdint>

// Existing RoF2 mapping/codec, not a newly discovered opcode.
// Client constructor 0x511990, executable SHA256
// 2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e.
// Signed incline is observed at record+48 -> float switch+0x38/+0x54.
// Unknown bytes are retained verbatim; this is not an executable packet sender.
namespace rof2_xorbb_protocol {
constexpr std::uint16_t OP_SpawnDoor = 0x7291;
constexpr std::uint16_t OP_MoveDoor = 0x08e8;
constexpr std::uint16_t OP_ClickDoor = 0x3a8f;
#pragma pack(push, 1)
struct DoorRecord {
    char model[32];                    // 0
    float y;                           // 32
    float x;                           // 36
    float z;                           // 40
    float heading;                     // 44
    std::int32_t incline;               // 48; raw native tilt, not degrees
    std::uint32_t size;                 // 52; 100 = normal model scale
    std::uint8_t unknown_56[4];         // 56
    std::uint8_t door_id;               // 60; native client door ID
    std::uint8_t open_type;             // 61
    std::uint8_t state_at_spawn;        // 62
    std::uint8_t invert_state;          // 63
    std::int32_t door_param;            // 64
    std::uint8_t unknown_68[12];        // 68
    std::uint8_t unknown_80[8];         // 80
    std::uint8_t unknown_88[12];        // 88
};
struct ClickDoor {
    std::uint8_t door_id;
    std::uint8_t unknown_1[3];
    std::uint8_t pick_lock_skill;
    std::uint8_t unknown_5[3];
    std::uint32_t item_id;
    std::uint16_t player_id;
    std::uint8_t unknown_14[2];
};
struct MoveDoor {
    std::uint8_t door_id;
    std::uint8_t action;
};
#pragma pack(pop)
static_assert(sizeof(float) == 4, "RoF2 wire float requires four bytes");
static_assert(sizeof(DoorRecord) == 100, "RoF2 door record size");
static_assert(offsetof(DoorRecord, incline) == 48, "RoF2 door incline offset");
static_assert(offsetof(DoorRecord, size) == 52, "RoF2 door scale offset");
static_assert(offsetof(DoorRecord, door_id) == 60, "RoF2 native door ID offset");
static_assert(sizeof(ClickDoor) == 16, "RoF2 click door payload size");
static_assert(offsetof(ClickDoor, item_id) == 8, "RoF2 click door item offset");
static_assert(sizeof(MoveDoor) == 2, "RoF2 move door payload size");
} // namespace rof2_xorbb_protocol
