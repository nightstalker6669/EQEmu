# Persistence and transaction contracts

Four additive migrations define seven tables, all InnoDB:

| Table | Key and contents | Lifecycle |
|---|---|---|
| `fellowship_guard` | Singleton id1 | Shared serialization point |
| `fellowships` | Auto ID; leader, MOTD, revision, creation time, camp zone/instance/type/XYZ/heading/expiry | Deleted on final departure/disband; camp expiry interpreted at read time |
| `fellowship_members` | Character PK; group FK, share flag, joined_at; group index | One membership per character; cascade with group |
| `fellowship_invitations` | PK target/inviter; group FK, expiry | Session-bound offer; group cascade |
| `fellowship_vitality` | Character PK; normal u64/AA u32 pools and observed caps | Survives leaving/disband; cleared for character deletion |
| `fellowship_xp_commits` | Receipt ID; character, XP/AA before/after, AA points before/after, timestamp | Latest256 receipts per character; audit, not replay |
| `fellowship_campsite_catalogue` | Recipe ID, description/name, kit/lumber, effect/amount/limits/template, enabled/source |597 published definitions, mirrored by compiled catalogue |

`Service` methods require no enclosing transaction. Mutation transactions verify connection/in-transaction state and hold guard1. They validate account/character existence, deletion status, membership/leadership, bounded counts and camp coordinates/lifetime. `SetCampfire` can receive a material callback and expected group+revision; stale preflight rejects instead of charging against changed membership.

## Paid camp transaction

The zone stages actual item pointer, serial, charges, slot and ID, then snapshots its held inventory admission lease. A dedicated configured connection runs `Service::SetCampfire`. Under its guard, the material callback rechecks actor/account, active session epoch/token/process/client/zone/instance and pending custody/coin/house operations. Exact raw inventory rows, null flags, GUID/custom/augment data and stack/item properties must match staged runtime identity. Two different slots are required. Both one-item debits and campsite persistence commit together.

Before commit there are no client inventory packets or quest callbacks. After proven commit the zone updates memory/packets without a second SQL write. If commit outcome or postcommit runtime ownership is uncertain, the session is rejected and disconnected to force authoritative reload. It never retries an uncertain purchase.

## Combat XP transaction

The combat hook runs after normal Lua XP overrides and only for non-resurrection kill XP with an NPC. It computes original earned deltas, validates caps, and invokes `VitalityService::ApplyCombat`. Bank allocations/redemption, final character XP/AA/progression fields and a journal receipt are persisted on that same verified connection/transaction. Recursive `SetEXP` skips redistribution and duplicate Lua overrides. Ordinary character/AA saves are suppressed during finalization.

The finalizer verifies the original character progression before its narrow update. It changes XP/AA/progression fields, not unrelated inventory, currencies or buffs. Connection ID and transaction checks prevent silent reconnect from committing only half an operation. Failure after runtime progression touched blocks subsequent saving and disconnects for authoritative reload. Unknown COMMIT is not replayed. The latest 256 audit receipts are retained per character; there is no pending-award replay worker.

This contract covers the integrated progression write and bank mutations. Custom Lua/quest hooks that independently commit other tables, issue external calls or perform their own durable side effects need their own transaction design. The finalizer cannot retroactively roll those back.

## Housing authorization

Personal owner/actor membership is resolved inside current ACL/scene/escrow transactions, under the same guard as membership mutations. Missing all fellowship tables yields no fellowship grant for older housing installs; partial schema is a failure, not an elevated grant. Guild property receives no inferred leader association. Native estate fellowship IDs are display data from a consistent snapshot and are not authority for item or money access.
