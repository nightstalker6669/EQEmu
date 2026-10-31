# Install `rof2-integration`

This checkout already contains the native source integration. **Do not run `apply_overlay.py` or `git apply` again.** Keep backups of the database, quests, binaries and active opcode configuration together before installing. No running server was modified by publishing this branch.

```sh
git -c core.autocrlf=false clone --branch rof2-integration --single-branch https://github.com/nightstalker6669/EQEmu.git
cd EQEmu
git submodule update --init --recursive
```

## Current upstream foundation

All feature branches now start directly from upstream `develop` commit **`c9c8747ac949deddb140d196d241fa4e777500c7`**, with no master merge in their new history. `master` remains the separate stable upstream branch. See [the branch map](BRANCHES.md), [clean comparisons and clone updates](COMPARE.md), and [exact provenance](integration.json).

Before applying feature migrations to an older database, build the matching current `world` binary through the normal upstream/vcpkg workflow and run its database updater from the configured server directory (`world database:updates`, or `world.exe database:updates` on Windows). Back up first and keep game services stopped. Review and complete the updater before the feature installation below. The housing migration ledger is not a replacement for upstream schema upgrades.

Current upstream uses vcpkg, CMake 3.20+, and a C++20 compiler. Initialize submodules before configuring. For native Windows, use the current upstream Visual Studio/vcpkg toolchain; for Akk-stack, use a build container with those dependencies and the correct mounts. System libraries alone are not a substitute for the current dependency manifest. The integrated installer keeps upstream precompiled headers enabled, matching the supported source build.

## Shared core is already included

This branch inherits [rof2-shared-core](https://github.com/nightstalker6669/EQEmu/tree/rof2-shared-core). Its canonical implementations are in `common/shared_core`; legacy housing headers are forwarding adapters. No second core overlay is needed. The housing installer applies the same existing session DDL through its original migration identity, preserving durable table names/history. Do not create a second renamed lease store or replay schema by numerical sorting.

## 1. Install housing infrastructure and content

Housing is a required implementation dependency for this branch's inventory/session guards. A PEQ fork is unnecessary: use an existing supported PEQ/MariaDB database plus the included migrations. Read the [complete housing prerequisites](../../extensions/rof2/housing/docs/INSTALL.md), including same-database requirements, matching original RoF2 resources and stopped writers.

From this repository root, first plan without `--apply`:

```sh
python3 -B extensions/rof2/housing/tools/install_housing.py \
  --integrated-source --source "$PWD" --server /srv/eqemu/server \
  --client /srv/clients/RoF2 --build /srv/eqemu-build \
  --backup /srv/backups/rof2-install-01
```

Replace the paths. On native Windows use `py -3 -B`, explicit Windows paths and the generator/toolchain options in the [Windows guide](../../extensions/rof2/housing/docs/UPGRADE-WINDOWS.md); add `--integrated-source` there too. For [Akk-stack](../../extensions/rof2/housing/docs/UPGRADE-AKK-STACK.md), run inside a one-off build container with the branch, server and private client resource mounts while normal game processes are stopped.

Repeat with `--apply` and a fresh backup directory after stopping world and every zone. **Always include `--integrated-source`.** It requires a clean committed checkout, clones that exact commit, builds all of its integrated features and deploys its full RoF2 mapping. It does not fall back to a housing-only source build. Do not pass a start command yet: companion schemas/quests below must be installed before any game process starts.

The housing driver uses the explicit [`installer-migrations.json`](../../extensions/rof2/housing/installer-migrations.json) order, not filename sorting. It installs the complete housing production schema, priced neighborhood, geometry, item/capacity profiles, NPC services, quests and reviewed trophy content. Public images remain linked to the pinned original release.

## 2. Install companion schemas and quests

Run the following groups **in this order**. Use your normal protected MariaDB credentials and stop on the first SQL error. Back up and review existing custom schema/content before applying.

- **Fellowship:** `utils/sql/fellowship/001_fellowships.sql`, `002_vitality.sql`, `003_xp_commits.sql`, `004_campsite_catalogue.sql`. Install the bundled `extensions/rof2/fellowship/quests/poknowledge/` quest override after reviewing conflicts. Follow [Database and content requirements](../../extensions/rof2/fellowship/docs/INSTALL.md#database), including matching camp spell/item templates.
- **Mercenaries:** provision the normal mercenary catalogue first; do not replay the destructive upstream bootstrap over owned contracts. Apply `utils/sql/mercenary/001_owned_roster.sql`, `003_passive_upkeep.sql`, `004_zone_permissions.sql`, then review/apply `002_enable_rof2.sql` for the actual ruleset and database layout. The [content and activation guide](../../extensions/rof2/mercenaries/docs/INSTALL.md#content-schema-and-activation) explains catalogue prerequisites and price rules.
- **Spirit Shrouds:** apply exactly `001_storage.sql`, `003_parity_storage.sql`, `004_live_imp_wizard.sql`, `005_experience_settlement.sql`, `006_starter_inventory.sql`, `007_rof2_enumerable_aa.sql`, `008_starter_identity.sql`, `009_live_captured_catalog.sql`, `011_live_lower_level_forms.sql`, `012_live_paid_monster_rank_chains.sql`, `013_remaining_monster_aa.sql`, `014_death_resurrection_accounting.sql`, `015_system_interactions.sql` from `utils/sql/shroud/`. Omitted numbers are intentionally not migration inputs. Follow [activation and recovery requirements](../../extensions/rof2/spirit-shrouds/docs/INSTALL.md#offline-migration-and-deployment); keep native transitions at their default false for first acceptance. Preserve escrow/snapshots and drain existing sessions normally.

## Install achievements and all reviewed quest content

The nine published achievement engine patches are already integrated. **Do not run their source patch commands.** The current updater registers `utils/sql/git/2026_07_28_achievement_content.sql` and `2026_07_28_achievement_state.sql`; inspect updater history rather than blindly replaying them. Apply the required follow-up `utils/sql/git/2026_10_27_achievement_reward_classes.sql` before starting the new binaries.

Then install every reviewed content package from the [achievement release installation guide](../../extensions/rof2/achievements/README.md#install), using its guarded preflights, complete required SQL sets and quest installers. The dependency order is Crescent Reach; the Blightfire Moors foundation and all five Blightfire quest-family packages; Stone Hive; the required Blightfire parent-reward upgrade; Mesa after its Crescent/Bixie/Stone Hive prerequisites; the complete Mesa armor, gifts and encounters extension; then Blackfeather Roost with its complete six-file SQL set, quest modules, preserving global hooks and combined nest engine support. Shard’s Landing is also included and uses the same engine/schema foundation. The individual guides specify exact file order and conflict handling. Preserve existing custom quest scripts for a deliberate merge. The later public achievement milestones are Mesa `a21ffd63` and Roost `2b7d6944`; these supplement the original input table below. Follow each package guide rather than replaying historical engine patches. Roost Ascending wrist and Raider glove/bundle rewards remain unconfigured.

The released content does not claim every achievement through Rain of Fear is playable. Its disabled/unverified definitions stay disabled; this upstream update does not invent missing objectives or rewards. All five feature packages and their known limits are included in the combined branch.

## 3. Start the matched installation

Only after all required schema/content/quest groups succeed, start the server through its normal service manager. All world and zone processes must use the newly built cohort, cache and mapping. Check startup and perform representative native acceptance for each enabled feature. Preserve the installation receipt and matched rollback backup; publication itself does not deploy a server. A separate controlled 2026-10-30 migration passed a full Linux combined build/link, matched deployment, data-preservation checks and representative RoF2 gameplay. This does not establish full cross-feature or native Windows acceptance; see the scoped migration entry in the integration manifest. Original source-rebase validation remains separately recorded.

The guarded personal housing marker orientation correction is available at `utils/sql/housing/2026_10_22_housing_brimming_personal_marker_orientation.sql`. Inspect its preconditions and back up affected rows before deliberate application; it is not a new mandatory migration for already corrected databases.

## References

| Feature | Published input | Detailed prerequisites / policy |
| --- | --- | --- |
| housing | [`710ecb7`](https://github.com/nightstalker6669/eqemu-rof2-housing/tree/710ecb7530ff0a5ade837e887ad7bc2f84622d07) | [Guide](../../extensions/rof2/housing/docs/INSTALL.md) |
| fellowship | [`9382453`](https://github.com/nightstalker6669/eqemu-rof2-fellowship/tree/9382453264f4f84ac7e83a9ee5c9c9a1391c6017) | [Guide](../../extensions/rof2/fellowship/docs/INSTALL.md) |
| mercenaries | [`00b07c3`](https://github.com/nightstalker6669/eqemu-rof2-mercenaries/tree/00b07c381a738a21aff08867bf186e6c2bfaffb0) | [Guide](../../extensions/rof2/mercenaries/docs/INSTALL.md) |
| achievements | [`334a0e2`](https://github.com/nightstalker6669/eqemu-rof2-achievements/tree/334a0e2c28079af07c9a4d07a0654f74ba9e4991) | [Guide](../../extensions/rof2/achievements/README.md#install) |
| spirit-shrouds | [`e32ca15`](https://github.com/nightstalker6669/eqemu-rof2-spirit-shrouds/tree/e32ca155a67b6f4d12eef36de131c5810998517a) | [Guide](../../extensions/rof2/spirit-shrouds/docs/INSTALL.md) |

[Opcodes and structures](PROTOCOL.md) · [Integration manifest](integration.json) · [Original upstream README](UPSTREAM-README.md)


## Verification

See [integration.json](integration.json) for original rebase validation and the separate completed runtime migration. The latter preserved all 437 tables and checked original values in affected tables; it is not a blanket native acceptance claim. No live server or database change is performed by source publication.

Steppes adds two required schema migrations and a complete guarded content installer. Follow the [Steppes engine section](../../extensions/rof2/achievements/ENGINE.md#steppes-shared-task-and-scene-integration) and [content guide](../../extensions/rof2/achievements/content/steppes/README.md). Back up and stop writers; deploy the matching rebuilt engine before players reconnect.

## Short Sharp Sting content upgrade

After the existing Stone Hive foundation, install the complete [Short Sharp Sting package](../../extensions/rof2/achievements/content/short-sharp-sting/README.md). Use its guarded installer for all five SQL phases and all quest callbacks together. Rebuild and deploy this branch’s matching engine first; the journal requires the recovered RoF2 container decoder. The package includes Linux, native Windows and Akk-stack directions and targeted backups. Windows execution remains unverified.
