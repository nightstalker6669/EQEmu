# Veil of Alaris: Pillars and Windsong travelers

This update enables the existing Pillars of Alra and Windsong Sanctuary traveler achievements and normal access to those two zones. It changes two zone admission values and enables two existing achievement definitions. It adds no NPC, task, reward, opcode or criterion row.

| Achievement | Current readiness |
| --- | --- |
| The Pillars of Alra Traveler (1973000) | Ordinary spell entry, completion and one normal relog accepted |
| The Windsong Sanctuary Traveler (1973100) | Configured and enabled; native entry/return still pending |
| The Sepulcher of Order Traveler (1973300) | Held; excluded from this update |

The Pillars check used native Alra Gate with an explicitly preowned QA spell, Admin0 and GM mode off. The spell's original acquisition quest is not included. The required component completed once and persisted without changing its count, completion timestamp or version. The Windsong approach was held because a safe visible route through the upper lifts was not established; there was no forced movement or GM teleport.

## Install

1. Install the achievement engine and its existing RoF2 catalogue first; see [engine requirements](../../ENGINE.md).
2. Back up your database. Arrange a maintenance window with players logged out.
3. Apply [010-pillars-windsong-travelers.sql](sql/010-pillars-windsong-travelers.sql) to the content database. It validates the existing identities, policies and transactional tables, then applies the complete two-zone update in one transaction. Stop if a guard rejects your installation. Do not drop an existing routine to force it through.
4. Restart the server normally so world and zone caches read the new admission data. If your binaries include the shared-core idle ZoneData fix, the supported `#reload zone_data global` and `#reload achievements global` commands can refresh the caches; verify actual world and every zone process completion. This hot-reload fix was deployed and verified locally with five idle zones and one loaded zone. Do not assume older binaries refresh idle caches.
5. Check the two achievements in game using an ordinary character. Windsong remains a native-validation limitation, not a claim of a completed route.

Rerunning the SQL accepts its exact supported postimage. Custom policy changes, duplicate zone identities, different criteria or unexpected reward entries are refused. Existing level, expansion, flag, instance membership and task gates are preserved. City of Bronze, Sepulcher, the Explorer parent and other children are not enabled by this update.

## Protocol and provenance

[RoF2 opcodes and packet structures](../../protocol/README.md) remain available in the protocol directory. This update uses the existing achievement ZoneEnter evaluation path and introduces no new wire protocol.

The [historical admission fragment](historical-admission.fragment.json) records dated 2012 sources and separates configured status from native acceptance. [Status](status.json) records the bounded local deployment and acceptance result. The fragment is not automatically merged into another server's admission allowlist.
