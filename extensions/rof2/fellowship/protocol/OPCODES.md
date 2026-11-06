# Fellowship opcode inventory

Target: **May 10, 2013 RoF2**, artifact SHA-256 `2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e`. Six mappings are confirmed from the matching shipped client. Names are emulator canonical names; values are client wire opcodes, separate from internal server opcodes.

| Canonical name | Wire | Direction | Server payload | Client evidence |
|---|---|---|---:|---|
| `OP_FellowshipAction` | `0x7121` | client to zone | 1076 | 0x669440 notify; 0x668710 confirmation |
| `OP_FellowshipUpdate` | `0x6e4d` | zone to client | 2564 | 0x4c55cd dispatch; 0x4b3860/0x4b3890 copies |
| `OP_FellowshipInvite` | `0x62ab` | zone to client | 1076 | 0x4c5733 dispatch; 0x667db0 invite UI |
| `OP_FellowshipCampRequest` | `0x0c12` | client to zone | 8 | 0x6688e0 request, literal at0x668967 |
| `OP_FellowshipCampList` | `0x074d` | zone to client | variable | 0x4d19e6 dispatch; 0x6691d0 parser |
| `OP_FellowshipVitality` | `0x2cc6` | zone to client | 32 | 0x4cd4d3 dispatch; 0x4b7110 consumer |

## Installation and routing

The integration patch adds canonical registrations and appends these mappings to `utils/patches/patch_RoF2.conf`. Deploy them to the active runtime mapping file. World and zone must be rebuilt together because canonical enum registration and internal messages change. New native packets use byte codecs directly; no unrelated petition encoder/decoder changes are required.

Only action and catalogue request are incoming connected handlers. Updates, invites, catalogue and vitality are outbound. `/fsay` is channel21 in existing `OP_ChannelMessage`; camps reuse normal spawn and `OP_Buff` protocols; insignia uses the existing item/spell/movement paths. They are not additional fellowship-specific numeric mappings.

**Size confidence differs by message.** Action1076 and request8 are established by native outgoing construction. Update2564 is the full copied extent, not a discovered receive length equality check. Invite1076 and vitality32 are emulator emitted sizes accepted by native gameplay; their receiver's exact minimum size is not established. Catalogue is variable. See [STRUCTS.md](STRUCTS.md) and [EVIDENCE.md](EVIDENCE.md).

Machine-readable inventory: [opcodes.json](opcodes.json). Unresolved fields are explicitly marked in [recovered-layouts.json](recovered-layouts.json); no uncertain nearby opcode is included as a proven mapping.
