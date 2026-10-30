# RoF2 achievement and reward protocol

The opcode mappings, fixed-prefix structures, variable layouts and production serializers are included. These mappings target the **May 10, 2013 RoF2 client**, not today's Live client. The protocol implementation originates in [EQEmu PR #5119](https://github.com/EQEmu/EQEmu/pull/5119) by **fryguy503**. This release adds reviewed reward eligibility, common-only claiming and Crescent quest dependencies.

This document describes the implemented protocol. Opcode presence does not establish that every achievement, original requirement, reward or associated quest is restored. The historical catalogue is partial, and catalogue identity coverage differs from playable content coverage.

## Opcode map

| Direction | Opcode | Value | Payload |
|---|---|---|---|
| C→S | AchievementRequest | `47CB` | Empty or one byte `30`; request achievement window |
| S→C | AchievementDefinitions | `DAB0` | Compressed categories and definitions |
| S→C | AchievementUpdate | `618F` | Primary dense/sparse state |
| S→C | AchievementProgress | `2B42` | Count plus 20-byte progress records |
| S→C | AchievementWindow | `171E` | Zero-payload window-open signal |
| S→C | AchievementState | `059D` | Comparison snapshot in definition order |
| S→C | AchievementEarned | `17F8` | Spawn, achievement, sound and link metadata |
| C→S | AchievementCompareRequest | `1665` | Two spawn IDs, 8 bytes |
| C→S | AchievementLinkRequest | `675D` | Player, achievement and definition-sized state |
| S→C | AchievementComparisonReply | `28E0` | Authoritative comparison state |
| Both | AchievementReward | `6411` | Claimable reward manager |
| Both | RewardSelection | `6471` | Task preview reward manager |

The adjacent `.conf` contains the exact `OP_` names. Merge it with the server's full mapping file; it is not a replacement for unrelated opcodes.

## Encoding

Integers are little endian, strings are NUL terminated (`zstr`). Arrays have no implicit C++ alignment. `rof2_achievement_packets.h` documents fixed prefixes only. Production code serializes variable records using `SerializeBuffer`; copying host `std::string` or `std::vector` objects into a packet is invalid.

**Definitions (`DAB0`)** start with `u32 inflated_byte_count`, then a zlib stream. The inflated body is `u32 category_count; Category[]; u32 achievement_count; Definition[]`.

A Category is `u32 id, parent; zstr name, description, icon; u32 order; u32 association_count; Association[]; u32 child_count; u32 child_id[]`. A root parent is `FFFFFFFF` on the wire. Association is `u32 achievement_id; zstr text; u32 order`.

A Definition is `u32 id; zstr name, description; u32 icon; u8 persistent; u32 version`, followed by four component vectors in type order **0,1,2,3**. Each vector starts with `u32 count`; each Component is `u32 id; u8 type; u32 required_count; zstr name, description; u8 order`. The definition ends with `u32 points; u32 has_reward`.

**State** is `i16 status` (completed0, open1, locked2, hidden3), followed by bitsets in type order **1,2,0**. For each type with `n` components, write `ceil(n/16)` little-endian `u16` words. A completed state then has `u32 timestamp`. Comparison records additionally append a `u32 current_count` for every component, again in **1,2,0** order. Type3 components have no state array in this protocol.

`059D` contains one State per definition, with no IDs/count prefix. It belongs to the comparison path. It must not replace the primary login state. `618F` contains `u32 serial; u8 dense; u32 count`, then State records. Dense records follow definition order; sparse records prefix each State with `u32 definition_index`.

`2B42` contains `u32 count` and fixed **20-byte** ProgressRecords: `u32 achievement_id, component_id, requirement_id, requirement_type, current_count`. Keep requirement keys stable between initial and live updates. This wire stride is separate from client in-memory component structures.

`17F8` contains `u32 achiever_spawn_id, achievement_id, sound_id; zstr caret_metadata`. The usual sound is3695. Metadata includes state/bitwords/counts in **1,2,0** order and ends in a caret. It does not include the chat `0x12` envelope that the client adds.

`1665` is `u32 target_spawn_id, requester_spawn_id`. The server authenticates the requester and checks actual local target/range. `675D` and `28E0` contain `zstr player_name; u32 achievement_id; State(with counts); u8 trailing_flag`. Their exact size depends on the authoritative definition and status. The server does not trust client-supplied completion state.

## Reward requests and display

Both reward opcodes begin with `u32 action`: List0, Inspect1, Claim3, TaskView4, AchievementView5, Pending6, Bulk7. Task/AchievementView requests have one further `u32 source_id`; Pending has no further fields. Inspect is20 bytes total: `action,set_id,option_id,reward_entry_id,item_id`. Claim is20 bytes: `action,pending_reward_id,set_id,selected_wire_option_id,unknown`. The fifth Claim DWORD is not assigned proven semantics.

Claim replies are20 bytes: four `u32` values (`action3,pending_id,set_id,subset_id`), then `u8 success` and three zero bytes. `FFFFFFFF` is the common-only selection sentinel when no selectable options exist. A normal wire option ID must resolve through the authenticated pending session. Pending IDs, source/task IDs and reward-set IDs are distinct. Class masks are **server-authored eligibility**, not extra wire fields; selection uses the permanent class.

A single display starts `u32 action0; u8 has_set` and optionally one Set. A bulk display starts `u32 action7; i32 set_count; u8 zero`, then Sets. Set is `u32 pending_id,set_id; zstr title; u32 multiplier_bits; u32 repeated_set_id; i32 subset_count`. Multiplier defaults to IEEE7541.0 (`3F800000`). Subset is `u32 subset_id,entry_count; u8 common_to_all,flags; zstr option_label; Entry[]`.

Entry is `u32 wire_type; u32 fields[2]; zstr description`, then a type-specific tail: Money1/GenericPoints11 use four `u32` values; Item2 uses `u32 item_count` and items `{u32 field0,field1;u8 field2;u32 field3;zstr text}`; AAAbility4 uses two `u32`; AAPoints5 uses `u32 value;u8 flag`; Text0/Experience3 have no additional serialized tail. Unknown field names are retained deliberately.

The claimable6411 lane rejects TaskView; preview6471 rejects Claim and AchievementView. Reward granting remains server authoritative, with durable pending/selection records and inventory-persistence checks.

## Source and validation boundaries

The patch series includes the canonical implementation in `common/achievements.{h,cpp}`, `common/reward_selection.h`, `zone/client_packet.cpp`, `zone/client_achievements.cpp`, `zone/reward_selection.cpp`, and `common/patches/rof2.cpp`. `serializer-extract.cpp.txt` is a convenient source excerpt, not a separate build unit.

Private native checks on the running port demonstrated achievement state/progress, earned notifications, class-filtered task reward selection, common-only achievement rewards and persistent grants. The clean upstream publication port has separately passed patch application checks. A full build/native run of the clean publication port has **not** been performed as part of the offline packaging audit. No client binaries, raw client catalogues, private character records, captures or test files are included.
