#pragma once
#include <cstdint>
// Existing RoF2 layouts copied by field from the retained engine declarations
// and GroundSpawn encoder. Little-endian wire integers/floats. These are not
// freshly captured Live layouts and are not server Object_Struct memory views.
#pragma pack(push, 1)
struct Rof2ClickObject {
    std::uint32_t drop_id;       // 0; ingress zone Object ID
    std::uint32_t player_id;     // 4
};
struct Rof2ClickObjectAction {
    std::uint32_t drop_id;
    std::int32_t unknown04;      // existing encoder writes -1
    std::int32_t unknown08;      // existing encoder writes -1
    std::uint32_t type;
    std::uint32_t unknown16;     // existing encoder writes 0
    std::uint32_t icon;
    std::uint32_t unknown24;     // retained as unknown
    char object_name[64];
};
struct Rof2GroundSpawnPrefix { std::uint32_t drop_id; };
// A variable-length NUL-terminated model name follows Prefix, then this Suffix.
// The encoder's packet allocation can contain additional trailing bytes; do not
// infer the complete packet size from this known-field prefix/suffix alone.
struct Rof2GroundSpawnKnownSuffix {
    std::uint16_t zone_id, zone_instance;
    std::uint32_t object_id_repeat, reserved;
    float heading, tilt_x, tilt_y, scale, y, x, z;
    std::uint32_t object_type;
};
struct Rof2TaskActivityComplete {
    std::uint32_t task_index, task_type, task_id, activity_id;
    std::uint32_t task_completed, stage_complete;
};
#pragma pack(pop)
static_assert(sizeof(Rof2ClickObject)==8, "RoF2 object-click layout");
static_assert(sizeof(Rof2ClickObjectAction)==92, "RoF2 container action layout");
static_assert(sizeof(Rof2GroundSpawnKnownSuffix)==44, "Known spawn fields");
static_assert(sizeof(Rof2TaskActivityComplete)==24, "Task completion fields");
