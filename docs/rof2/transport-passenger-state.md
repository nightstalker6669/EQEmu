# Transport passenger state

Mer's original Maiden's Grave mission requires a ticket and a ship voyage.
Quest scripts need the passenger vehicle reported by the player's movement
packets. Standing near a ship does not establish boarding.

## Existing RoF2 opcode and structure

This uses the existing `OP_ClientUpdate = 0x7dfc` mapping in
`utils/patches/patch_RoF2.conf`. No opcode, wire structure or client asset is
added or changed.

Both `common/eq_packet_structs.h` and
`common/patches/rof2_structs.h` define
`PlayerPositionUpdateClient_Struct` with the following fields:

| Byte offset | Field | Representation |
| --- | --- | --- |
| 0 | sequence | uint16 |
| 2 | spawn_id | uint16, player entity ID |
| 4 | vehicle_id | uint16, passenger vehicle entity ID |
| 6 | unknown0004 | four bytes; meaning unchanged |
| 10 | delta_x | float |
| 14 | heading / padding0040 | 12 / 20 bit unsigned fields |
| 18 | x_pos | float |
| 22 | delta_z | float |
| 26 | z_pos | float |
| 30 | y_pos | float |
| 34 | animation / padding0024 | 10 / 22 bit unsigned fields |
| 38 | delta_y | float |
| 42 | delta_heading / padding0041 | 10 bit signed / 22 bit unsigned fields |

The declared structure is 46 bytes. The existing handler also accepts its
47-byte compatibility form; this work does not assign meaning to that extra
byte. While aboard, position fields are vehicle-local offsets. The existing
handler rotates/translates them using the boat's position and heading.

## Script API

`Client:GetPassengerBoatID()` returns the recent passenger vehicle entity ID,
or zero when none can be established. It observes a structurally accepted
player movement packet referencing an existing ship or controllable boat.
An on-foot update clears it. It expires after 15 seconds without a qualifying
update, and the getter rechecks that the referenced entity is still a boat.
The expiry is a server consistency policy, not a recovered historical timer.

This is read-only script access. It does not board, move, steer or grant
control of a boat. The existing C++ `GetBoatID()` and its controllable-rowboat
behavior remain separate.

Mission scripts must also validate the exact original ship/spawn, the active
mission and its stage, carried ticket custody and the player's task-owned
dynamic zone. Vehicle reports are client input, not independent proof of
physical collision. A mission must not replace these checks with a distance
test or create a new instance on every movement update.

Full voyage, ticket consumption and reconnect behavior require native
acceptance with the complete mission content. Compilation and freshness
checks alone do not prove that gameplay.

## Owned shared-task instance lookup

`Client:GetSharedTaskInstanceID(task_id, zone_id, zone_version)` returns the
single matching instance ID, or zero. It requires the active shared task and
its exact occurrence, a persisted membership in that task, a persisted
shared-task-to-dynamic-zone mapping, and a live mission instance with both
cached and database membership for the calling character. Expired/completed
occurrences, other tasks, other members, stale mappings and ambiguous matches
cannot establish a destination. The occurrence identity stays a native64-bit
integer in the query; Lua stores the matching identity as a string.

This is read-only lookup, not instance creation. Mer uses the native task
`dz_template_id` mechanism: world creates the mission instance after successful
admission. Scripts must not call `CreateTaskDynamicZone` per member or per
movement update, because that method can create additional instances.

## Historical average-level admission

Candidate schema9335 adds `shared_task_average_level_policies`. An enabled
policy replaces individual minimum-level gating with a documented party
average for a task whose `min_level` is zero. Requests and late additions use
fresh persisted levels for every resulting member. Unknown, missing or invalid
levels cannot be compensated for by high-level members. Comparisons use the
integer sum against `minimum * member_count`, without rounding down a mean.

Tasks without an enabled policy keep their existing admission behavior. Mer's
required content includes average70, three-to-six members and a six-hour task
and dynamic-zone template. Its task, policy, reward catalogue and scripts must
be activated together through the whole content installation.
