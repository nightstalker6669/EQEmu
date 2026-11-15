# Oceangreen progression — accepted release scope

This release adds the approved local reconstruction of ten Oceangreen tasks: six personal tasks and four shared missions. It combines native progression, final quest scripts, static content and additive database migrations. Use the matching source, server package and database contract together. It preserves the published character-selection crash fix and includes the measured interrupt-message packet writer repair.

## What passed native acceptance

All ten tasks completed on the matched local server. Stop the Contamination, A Dread Challenger, Freedom! and Beat the Blackburrow Boss each delivered the frozen reward once to each of two real recipients. Ordinary Hills/Blackburrow travel and mission returns passed. Earned progression admitted Void B, allowed an actual scroll purchase and native spell scribing, and allowed an ordinary level-100 Journeyman II healer hire. Retained history, currency, reward state and mercenary persistence were checked through actual fresh logins. Temporary administrative privileges were restored before publication.

Journeyman II acquisition uses committed completion of the four bound group-theme tasks, not account status or an invented flag. Existing owned mercenaries remain owned. Other mercenary tiers retain their existing policies. The `SodProgression:EnableOceangreenSlice` rule defaults off in source; the matching reviewed content installation enables the slice after its required schema and content are installed.

## Limits

Upper-ward physical blocking remains **LIMITED**. Standing access, line of sight and melee were observed, but the complete closed/open collision behavior was not qualified. Do not treat this release as global wall-geometry acceptance.

Encounter numbers, loot and AI parameters are approved local reconstruction, not authenticated original Live values. Optional Zeralin rendering and post-expiry repeat execution are not promoted to accepted behavior without retained native proof. The measured packet writer repair and its bounded regressions do not establish universal heap safety. Native acceptance used the matching Linux development cohort; CI compilation and packaging on another platform do not establish native gameplay acceptance there.

## Fresh installation

Follow the server package's included installation guide and its exact database reference. The matched sanitized full export is for a **new empty database only** and replaces tables. Never import it over player data. It preserves the existing published content and 331 portable housing JSON files, adds the accepted static Oceangreen content, and includes the six new runtime tables as empty schemas. It includes no player, QA, earned-history or private operational records.

The package applies the fifteen committed Oceangreen quest files over the pinned runtime content seed. Existing global player hooks are preserved; the encounter lives under `global/encounters/`. Original client archives/maps remain operator-supplied resources.

## Upgrade an existing server

Back up the database, installed files, private configuration and credentials together, and stop all game and administrative writers. Use the package's `database/migrations/oceangreen/UPGRADE-CONTRACT.json` and its matching ordered migrations. The reviewed path targets MariaDB 10.11, schema 9338 and the prior Mercenary86–100 seed lineage; a different schema, occupied feature namespace or failed precondition requires separate review.

1. Apply `001_oceangreen_required.sql` in a reviewed `peq_full` client session with `@sod_oceangreen_20261115_authorized=1`. It installs the additive schema/static content and activation while preserving existing player rows and unrelated custom content.
2. For a managed EverForge database, apply `002_everforge_accept_seed.sql` only after the first migration succeeds, with `@sod_oceangreen_20261115_marker_authorized=1`. It verifies the accepted content and advances the existing compatibility marker from the previous seed digest to the new one. It does not replay full SQL, adopt an unmanaged database, or certify mutable player data.
3. Install the matched native binaries and all fifteen quests together, retaining operator configuration and other quest files. Generate shared data as required by the normal stopped-server installation process, then start the matched installation.

Use normal private credential handling. Do not use a client `--force` option, weaken guards, clear feature state, or advance a marker by hand. DDL can commit independently: stop at an error, retain its terminal receipt and inspect the actual state before any recovery. Do not blindly retry a partially applied migration. A database that already contains this feature needs its own reviewed disposition.

Existing EverForge installations retain their installed server selection on restart. After the reviewed additive upgrade, deliberately install the matching published project release using Spire or the documented stopped-container CLI path. A restart alone does not update binaries, and the full seed must never be replayed to bypass an upgrade refusal. See [EverForge installation and persistence](https://github.com/nightstalker6669/EQEmu/blob/develop/docs/rof2/EVERFORGE.md).

Source copies of the ordered migration files and contract are under `utils/sql/sod/oceangreen/`; the matching database release carries the authoritative hashed inputs. Read its README and qualification documents before applying anything.
