# Internal world/zone messages

These are **new emulator-internal opcodes**, not recovered EverQuest client wire values. They live in `common/servertalk.h`. Rebuild/deploy world and zone together. Integer fields use the existing same-build ServerPacket ABI; do not expose these as client opcodes.

| Name | Value | Direction | Size |
|---|---|---|---:|
| `ServerOP_FellowshipRefresh` | `0x5125` | Zone → world → zones |0 |
| `ServerOP_FellowshipChat` | `0x5126` | Zone → world |1028 (`0x404`) |
| `ServerOP_FellowshipChatDeliver` | `0x5127` | World → recipient zone |1100 (`0x44c`) |

## Refresh

Zero payload only. World forwards through authenticated zone connections. Receiving zones invalidate camp state, reload current membership/invitations and refresh eligible housing catalogues/scenes. No claimed member/fellowship identity or native snapshot travels in the refresh.

## Chat request

| Offset | Type | Field |
|---|---|---|
|0 |u32 |Authenticated actor character ID |
|4 |char[1024] |Nonempty NUL-terminated message |

World verifies exact size, bounded terminator, the actor's current in-zone client-list entry and that this connection owns it. It reloads authoritative membership/name before constructing deliveries. Client text never supplies a sender name or recipient identity.

## Chat delivery

| Offset | Type | Field |
|---|---|---|
|0 |u32 |Actor character ID |
|4 |u32 |Recipient character ID |
|8 |u32 |Fellowship ID |
|12 |char[64] |Authoritative sender name |
|76 |char[1024] |NUL-terminated message |

Receiving zone validates size/terminators, locates the recipient, reloads current membership and checks sender ID/name in that fellowship. RoF2 receives channel21 CommonTongue. Other client versions get a text fallback. The native sender already echoes `/fsay`, so world excludes it from delivery to avoid duplicate text.
