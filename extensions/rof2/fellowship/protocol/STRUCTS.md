# Native fellowship packet structures

All offsets are **payload-relative**, excluding the2-byte opcode. Integers are little-endian, floats IEEE754 binary32. Wire strings are bounded NUL-terminated bytes. Use explicit byte operations; host C++ packing/alignment is not a protocol definition. The production codec zero-initializes unused bytes.

[Machine-readable layouts](recovered-layouts.json) enumerate all ranges and evidence classifications. [OPCODES.md](OPCODES.md) distinguishes exact sender sizes from chosen outbound sizes.

## Action envelope —0x7121,0x434 bytes

| Offset | Bytes | Type | Field | Meaning / evidence |
|---|---:|---|---|---|
| `0x0` | 4 | `u32` | `action` | Native action1..12 |
| `0x4` | 4 | `u32` | `target_id` | Spawn entity for invite; character ID for remove/promote/toggle |
| `0x8` | 4 | `bytes` | `reserved_08` | Zero emitted; semantics unresolved |
| `0xc` | 4 | `u32` | `inviter_spawn_id` | Read by incoming invite UI; not used as actor authority for client actions |
| `0x10` | 4 | `u32` | `fellowship_hint` | Native XP action copies player field; semantic meaning unresolved; ignored for authority |
| `0x14` | 1024 | `char[1024]` | `motd` | Action2 requires NUL within1024; maximum1023 content bytes |
| `0x414` | 20 | `bytes` | `reserved_414` | Zero emitted; semantics unresolved |
| `0x428` | 4 | `u32` | `campsite_id` | Action8 selected catalogue ID |
| `0x42c` | 8 | `bytes` | `reserved_42c` | Zero emitted; semantics unresolved |

The invite reply shares the action-sized envelope but only inviter spawn at+0x0c is proven consumed by that UI. Client actions use+4 differently; never treat an invited spawn ID as a persistent character ID. MOTD is **+0x14**, not the superseded+0x1c interpretation. Native bounded copy permits1023 content bytes plus NUL. Server rejects wrong length, action outside1..12 and unterminated MOTD for action2.

| Value | Action | Relevant argument / authority |
|---:|---|---|
| 1 | Create | none |
| 2 | SetMOTD | motd at0x14 |
| 3 | Accept | server binds to delivered inviter+expiry |
| 4 | Decline | server binds to delivered inviter+expiry |
| 5 | Invite | target_id is actual selected spawn entity |
| 6 | Remove | target_id is member character ID |
| 7 | TransferLeader | target_id is member character ID |
| 8 | CreateCamp | campsite_id at0x428 |
| 9 | DestroyCamp | none |
| 10 | Leave | leader must transfer or disband |
| 11 | Disband | leader authority |
| 12 | ToggleXP | target_id is member character ID; hint+0x10 untrusted |

The authenticated `Client::CharacterID()` supplies the actor. No client-declared fellowship ID, hint or target grants leadership. Invite requires the sender's actual nearby target; accept/decline is bound to the offer delivered to that connection. Native action8 sends the selected catalogue key, not the material item ID.

Illustrative payload prefixes: create `01 00 00 00`; invite target4660 starts `05 00 00 00 34 12 00 00`. These are byte examples, not packet captures. Fill the full1076-byte envelope and validate authority server-side.

## Member record —0x54 bytes

| Offset | Bytes | Type | Field | Meaning / evidence |
|---|---:|---|---|---|
| `0x0` | 4 | `u32` | `character_id` | Persistent character identity, not zone spawn ID |
| `0x4` | 64 | `char[64]` | `name` | NUL-terminated bounded member name |
| `0x44` | 4 | `u32` | `zone_id` | Current known zone |
| `0x48` | 4 | `u32` | `level` | Current known level |
| `0x4c` | 4 | `u32` | `class_id` | Class |
| `0x50` | 4 | `u32` | `last_on` | Server observed last login value; zero for connected local members |

## Roster and camp snapshot —0x6e4d,0xa04 bytes emitted

| Offset | Bytes | Type | Field | Meaning / evidence |
|---|---:|---|---|---|
| `0x0` | 4 | `u32` | `version` | Native constructor5; server5 |
| `0x4` | 4 | `u32` | `fellowship_id` | Current or previous ID for clearing update |
| `0x8` | 64 | `char[64]` | `fellowship_name` | Not synthesized; zero emitted |
| `0x48` | 1024 | `char[1024]` | `motd` | NUL-terminated |
| `0x448` | 4 | `u32` | `member_count` | 0..12 |
| `0x44c` | 1008 | `Member[12]` | `members` | Stride84; leader first |
| `0x83c` | 4 | `u32` | `sync` | Constructor default1; server1; revision is not wire Sync |
| `0x840` | 384 | `bytes[12][32]` | `player_handles` | Zero emitted; not fabricated |
| `0x9c0` | 12 | `u8[12]` | `sharing` | Per-member Boolean in roster order |
| `0x9cc` | 12 | `u8[12]` | `capped` | Per-member authoritative bank-cap admission flag |
| `0x9d8` | 12 | `u8[12]` | `offline_mode` | Zero emitted; last-on/zone still supplied |
| `0x9e4` | 4 | `f32` | `camp_y` | Native Y before X |
| `0x9e8` | 4 | `f32` | `camp_x` | Native X |
| `0x9ec` | 4 | `f32` | `camp_z` | Native Z |
| `0x9f0` | 4 | `u32` | `camp_zone` | Native UI zone resolver input |
| `0x9f4` | 4 | `u32` | `camp_timestamp` | Nonzero presence; server uses persisted epoch expiry; native clock origin unproven |
| `0x9f8` | 4 | `u32` | `camp_second_timestamp` | Zero emitted; semantics unresolved |
| `0x9fc` | 4 | `u32` | `camp_type` | Type label from exact-build source correspondence |
| `0xa00` | 1 | `u8` | `camp_present` | 1 for active camp |
| `0xa01` | 3 | `bytes` | `camp_padding` | Zero emitted |

The roster part is0x9e4 bytes; the camp suffix is0x20 bytes. The native receive path copies both extents. Member0 controls leadership UI. Exactly12 member slots are encoded, regardless of count; unused slots remain zero. Sharing/capped arrays use the same leader-first order.

A nonmember recipient clears its current fellowship only when packet ID matches its current ID. To clear stale camp state as well, the integration first sends a member-present, camp-absent snapshot, then a matching zero-member removal snapshot. For an active camp the server supplies epoch expiry as the nonzero timestamp marker, without claiming the client uses Unix epoch. Expired/absent camps emit a zero suffix.

Version5 and Sync1 reflect native constructor defaults. The internal database revision is never substituted for Sync. Fellowship name, handles and offline-mode bytes remain zero instead of fabricated values. No instance ID or heading is in the documented native camp suffix; the server retains those for authoritative travel.

## Invitation —0x62ab

Emitted size0x434, zero except inviter spawn/entity ID at+0x0c. The receive UI resolves that local entity to its name and displays confirmation. No exact receive minimum was recovered. Native acceptance against the emitted envelope was observed. Inviter is not a character-ID field on this message.

## Catalogue request —0x0c12,8 bytes

| Offset | Bytes | Type | Field | Meaning / evidence |
|---|---:|---|---|---|
| `0x0` | 4 | `u32` | `usable_only` | 0 show all;1 usable only; inverse of ShowAll flag |
| `0x4` | 4 | `u32` | `category_mask` | 0xffffffff all, otherwise1<<category_id |

Categories:1 General,2 Spell Focus,3 Melee Focus,4 Statistics,5 Resistances,6 Illusions,7 Buffs. Thus melee-only mask is8 (`08 00 00 00`); all categories is `ff ff ff ff`. `usable_only` must be0 or1. Availability also requires membership, proximity, no active camp, native template support and materials.

## Catalogue reply —0x074d,variable

Prefix is u32 record count. Each consecutive record has this **unaligned** layout:

| Offset | Bytes | Type | Field | Meaning / evidence |
|---|---:|---|---|---|
| `0x0` | 1 | `u8` | `usable` | 0 gray/unavailable;1 usable |
| `0x1` | 4 | `u32` | `campsite_id` | Selection key sent by action8 |
| `0x5` | 4 | `u32` | `description_id` | Native description-string ID |
| `0x9` | variable | `cstring` | `name` | NUL-terminated variable length; record size10+name byte length |

Total size `4 + sum(10 + name_bytes)`; there is no fixed stride, padding or extra per-record length prefix. Example name `X`, usable1, key1, description2: record bytes `01 01 00 00 00 02 00 00 00 58 00`. The NUL terminator advances the next record. The usable flag controls native enabled/gray presentation.

## Vitality —0x2cc6,32 bytes emitted

| Offset | Bytes | Type | Field | Meaning / evidence |
|---|---:|---|---|---|
| `0x0` | 8 | `u64` | `normal_pool` | Native64-bit read |
| `0x8` | 8 | `u64` | `normal_cap` | Native64-bit read |
| `0x10` | 4 | `u32` | `aa_pool` | Native low32-bit read |
| `0x14` | 4 | `u32` | `aa_pool_high` | Zero emitted; native consumer does not read this word |
| `0x18` | 4 | `u32` | `aa_cap` | Native low32-bit read |
| `0x1c` | 4 | `u32` | `aa_cap_high` | Zero emitted; native consumer does not read this word |

The encoder writes four8-byte slots, with AA values constrained to low32 bits and high words zero. The native consumer reads only low AA words; this does not prove they were formally64-bit fields in Live. Native gauges clamp pool to cap then compute thousandths. A zero high word is deliberate, not a recovered unknown-field meaning.

## General protocols reused

Channel21 in existing channel messages carries fellowship chat. Existing spawn/entity packets show the camp. Existing `OP_Buff` conveys Standard spell ID, effective caster level and renewed duration; buffs/appearance changes are not extra fellowship-specific message numbers. Item34023's spell 7618 invokes ordinary movement to the server-authorized camp destination. General packet structs remain in the upstream/pinned housing source.
