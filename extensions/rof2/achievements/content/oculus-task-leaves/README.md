# Oculus: seven existing task prerequisites

Status: **enabled locally; seven prerequisite facts and persistence verified in RoF2**. This package adds seven completion rules to the existing Model XLII Spatial Temporal Oculus achievement (500120700). It reuses the installed Dragonscale tasks. The remaining 48 requirements, Oculus quest journal, crafted-item assembly and final reward are outside this update.

| Required quest | Existing task | Oculus component |
| --- | --- | --- |
| I Can Fly | 3041008 | 51207029 |
| Construction Junction, What's Your Function? | 3041006 | 51207031 |
| Darwin's Award | 3041011 | 51207033 |
| Farm Clean Up - Crows | 3041001 | 51207035 |
| Farm Clean Up - Scarecrows | 3041002 | 51207036 |
| Farm Clean Up - Doombugs | 3041003 | 51207037 |
| Forgetful Nanzie | 3041004 | 51207039 |

## Install

1. Install the achievement engine/catalogue and the published Dragonscale task content first. Back up the database and arrange a maintenance window with content writers stopped.
2. Apply `sql/010-seven-existing-task-leaves.sql` with a MariaDB/mysql client. Do not use its continue-on-error option. This is package-owned content SQL, not a new global updater version.
3. Save the seven allocated criterion IDs printed at the end. The live table allocates IDs; no global range is assumed.
4. Refresh achievement caches using the supported reload command or a normal server restart. Verify each process receipt; SQL application alone does not prove cached or client behavior.
5. Verify the seven leaf requirements and a normal relog. Existing legitimate completed-task history can reconcile through `IsTaskCompleted`. Do not insert task history or infer those seven completions from owning a final Oculus item.

The installer validates full preimages for seven tasks, 23 activities, the existing achievement and all 57 components. It also checks associations and rewards, refuses unrelated Oculus criteria, and serializes package operations with a named lock. Exactly zero rules or the identical seven-rule postimage are supported. A conflicting or partially populated installation stops for review. It modifies only the seven criterion rows; tasks, activities, items, NPCs, player records and journal settings remain untouched.

## Rollback

Use the exact allocated IDs from the installation receipt. In the same SQL connection, set `@oculus_leaf_rollback_ids` to a JSON array containing those seven numeric IDs, then apply `sql/020-rollback-seven-leaves.sql`. The rollback checks the IDs and complete semantic row identities before removing the rules. It refuses changed content or a subsequently expanded Oculus configuration. It preserves player progress and task history. Refresh caches afterward.

If a guard fails, stop. Do not drop a preexisting procedure or change content merely to bypass the guard. Temporary tables are connection-local; use a fresh connection after the failed operation has been reviewed.

## Evidence and limits

The reused Dragonscale task implementation and prior native completion milestone were published in [the Dragonscale update](https://github.com/nightstalker6669/EQEmu/commit/5e459f6fef2874929a499876c649f3026ae174d4). The seven new Oculus rules were then checked using an ordinary character with those existing completed-task records: exactly seven requirements became satisfied, while the other 48 remained unsatisfied and the full achievement stayed open. The seven counts, timestamps and versions persisted unchanged across logout and one subsequent login. The seven rows match the existing full Oculus definition manifest except for their explicitly enabled state. The full 55-rule journal remains held.

[RoF2 opcodes and packet structures](../../protocol/README.md) are available separately. These rules reuse the existing TaskComplete event and add no opcode, packet structure or engine change.

The exact installer and rollback passed 12 cases in one disposable eight-table MariaDB fixture: fresh install, identical reapply, wrong-ID rejection, exact-ID rollback, changed task/activity/definition/component guards, partial/foreign rule rejection, named-lock contention, and a forced failure on the sixth insert. Negative cases left no open transaction; failed inserts rolled back. The fixture was removed. The running server also loaded the exact seven added rules across all six zone processes, with zero rebuild failures. Native component satisfaction and persisted SQL counts were checked separately: the RoF2 observer does not expose a verified numeric component counter.

During verification, login expiry removed three old summoned backpacks and four permanent contents. Acceptance was held. The four permanent item instances were recovered from their exact saved records into verified empty accessible slots, the client allocated fresh identifiers, and their metadata and persistence were verified before closeout. The expired backpacks were not recreated. The preservation result includes this corrective recovery; the achievement update does not change the existing container-expiry behavior. Private player records, recovery SQL and raw test evidence are not included in this package.

Verification lesson: check expiring containers and retain an inventory snapshot before logging in a character after a long absence.
