# Implementation and verification status

Published October 23, 2026. Matching native client: May 10, 2013 RoF2. The local combined source built and was deployed with matching world and all six supervised zones verified by executable hashes and health probes.

## Native gameplay observed

Three disposable local accounts exercised:

- Creation, invitations/acceptance, three-member roster, MOTD, leadership transfer and sharing toggles.
- Cross-client and cross-zone channel21 chat without duplicate sender echo.
- Basic camp create/destroy, usable paid recipe catalogue, category filtering and exact one-kit/one-lumber debit.
- Health camp150HP: native2374→2524 and server2671→2821; destruction restored native2374 and removed the scoped buff. An earlier failed manual-field implementation was corrected before final acceptance. The unrelated base client/server difference was present before the camp; the verified delta was150 on both.
- Faerune scoped buff application/destruction; nearby human members becoming barbarian and returning to their original appearance after expiry. Expiry was accelerated by shortening only the disposable fellowship's expiry under its guard; it was not an hour-long measurement.
- Camp model in world; member zoning to Nexus loses the aura, sends chat to PoK, clicks the insignia and returns to the actual camp, regaining150HP and starting its native15-minute cooldown.
- Normal combat sharing to an offline member and redemption. One donor kill retained180590XP and banked180589 for each other member. Recipient's next kill consumed that bank and awarded361179. Offline member's bank reached361178 and survived restart/relog.
- Native AA allocation: donor338606AA; each other member banked338604. Recipient's next kill redeemed its bank for677210AA; third member's pool reached677208. Exact character and journal values agreed.
- Leader removal and native end-fellowship confirmation clearing the remaining roster.

All three accounts, characters, associated inventories/banks/receipts/session data, private credentials manifest, disposable client profiles and the exact temporary NPC were removed after normal logout and guarded cleanup. No existing user credentials were submitted.

## Programmatic verification

The full597-entry catalogue and native signatures, packet codecs, exact material transactions, killed connections, rollback, concurrency, stale group/revision, XP finalizer/crash/reconnect/cap behavior, DBcore borrowed-handle reconnect, and actual housing ACL/scene/escrow revocation were checked. Relevant isolated checks ran with ASan/UBSan. These complement native samples; they do not mean all 597 recipes were individually played.

Publication additionally checks the exact public housing baseline, source package hashes, patch application, installed source equality and scope. Raw suites, fixtures, binaries, screenshots, generated reports and server logs remain outside this production repository.

## Limits

Exact Live policy equivalence remains unproven for radius, focus randomness, recipe-to-melee-level mapping, cross-level XP/normal-AA conversion and complete native timer semantics. Other client builds have not received native fellowship-window acceptance. See [KNOWN-GAPS.md](KNOWN-GAPS.md).
