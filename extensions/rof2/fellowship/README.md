# EQEmu RoF2 Fellowship

Native fellowship support for EQEmu targeting the **May 10, 2013 Rain of Fear 2 client**. Published October 23, 2026.

This production source extension includes recovered opcodes and packet layouts, membership and invitations, campsite recipes/effects, travel, durable normal/AA vitality, SQL migrations, and fellowship-aware housing permissions. It is an extension of a pinned EQEmu checkout, not a complete server distribution.

## Dependency: housing integration required

**This release requires [eqemu-rof2-housing](https://github.com/nightstalker6669/eqemu-rof2-housing) at commit `9aad4cc0857bdd14677dd45ab24f3eafe20d489b`.** Paid campsite creation reuses its inventory admission leases, raw inventory identity checks, and pending-operation guards. Fellowship membership also supplies the housing `_Fellowship` permission group. The permission changes are included here; the existing housing inventory infrastructure is supplied by that companion repository.

This is an implementation dependency, not an inherent EverQuest fellowship requirement. It is not a standalone fellowship patch for stock upstream EQEmu. No claim is made that disabling housing bypasses the dependency. See [installation](docs/INSTALL.md) and [architecture](docs/ARCHITECTURE.md).

## Supported behavior

- Create a fellowship, invite/accept/decline with five-minute expiry, maintain up to twelve members, transfer leadership, remove/leave/disband, and edit MOTD.
- Native fellowship window and channel-21 `/fsay`, including cross-zone delivery and membership refresh.
- All **597 campsite definitions**: one free basic camp and 596 paid variants, native category filters, usable-material checks, and exact kit/lumber debits.
- Camp world entities, one-hour expiry, nearby numeric/melee/focus/support/illusion effects, native buff projection, and insignia travel with ordinary instance admission checks.
- Normal and AA XP sharing, offline banks, redemption, caps, automatic sharing shutdown at cap, and native vitality gauges. Bank mutations and character XP commit together.
- Current fellowship-based personal housing access; membership changes revoke grants and refresh scenes across zones.

Three matching native clients exercised creation/invitations, MOTD/chat/leadership, paid materials, health/Faerune/illusion camps, expiry, filtering, travel, normal/AA XP, offline banking, restart/relog persistence, removal and disband. The complete catalogue was checked programmatically; individual gameplay did not exercise every recipe. See [status](docs/STATUS.md).

**Functional local support does not establish exact Live parity.** Radius, deterministic focus values, melee recipe tiers and XP conversion use documented emulator policies where native evidence is incomplete. See [known gaps](docs/KNOWN-GAPS.md) and [gameplay policies](docs/GAMEPLAY-POLICIES.md).

## Protocol reference

| Opcode | Name | Direction | Payload |
|---|---|---|---|
| `0x7121` | `OP_FellowshipAction` | Client → zone | `0x434` bytes; actions1–12 |
| `0x6e4d` | `OP_FellowshipUpdate` | Zone → client | `0xa04` bytes emitted; roster + camp |
| `0x62ab` | `OP_FellowshipInvite` | Zone → client | `0x434` bytes emitted; inviter spawn at+0x0c |
| `0x0c12` | `OP_FellowshipCampRequest` | Client → zone | 8 bytes |
| `0x074d` | `OP_FellowshipCampList` | Zone → client | Count + variable records |
| `0x2cc6` | `OP_FellowshipVitality` | Zone → client | 32 bytes emitted; normal/AA pools + caps |

- [Opcodes, direction, dispatch and confidence](protocol/OPCODES.md)
- [Complete packet structures, offsets, action values and examples](protocol/STRUCTS.md)
- [Machine-readable opcodes](protocol/opcodes.json) and [layouts](protocol/recovered-layouts.json)
- [Internal world/zone protocol](protocol/INTERNAL.md)
- [Recovery evidence and unresolved fields](protocol/EVIDENCE.md)
- Authoritative native codec: [rof2_wire.h](overlay/common/fellowship/rof2_wire.h)

## Install and repository layout

Start from upstream EQEmu commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39`, apply the pinned housing source overlay, then run:

```sh
python3 tools/apply_overlay.py /path/to/EQEmu --check
python3 tools/apply_overlay.py /path/to/EQEmu --apply
```

**Read [INSTALL.md](docs/INSTALL.md) first.** It covers exact dependency checkout, database schemas, native spell/item requirements, quest installation, building, deployment and rollback. Source installation does not migrate a database or restart a server.

| Path | Contents |
|---|---|
| `overlay/` | New production fellowship sources, housing membership adapter, and four additive SQL migrations |
| `patches/eqemu-fellowship.patch` | Fellowship integration and required DBcore reconnect fix against the pinned housing baseline |
| `patches/base-manifest.json` | Exact checksums of the prerequisite housing source overlay |
| `quests/poknowledge/` | Randall of the Fellows help and replacement insignia dialogue |
| `protocol/` | Opcodes, structures, internal messages and evidence limits |
| `docs/` | Installation, architecture, persistence, campsite policies, status and gaps |
| `tools/apply_overlay.py` | Validates package hashes and exact baseline before installing sources |
| `source-manifest.json` | Production publication file inventory and SHA-256 hashes |

## Publication and license

Production code, SQL, installation tooling and documentation only. Test suites/helpers/fixtures, generated test reports, logs, credentials, personal runtime configuration and client assets are excluded. Disposable verification accounts were removed from the development server.

EQEmu upstream GPLv3 is retained in [LICENSE](LICENSE); see [NOTICE](NOTICE). Independent community project; no affiliation with Daybreak Games. No client executable or proprietary game archive is distributed.
