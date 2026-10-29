# Install `rof2-spirit-shrouds`

This checkout already contains the native source integration. **Do not run `apply_overlay.py` or `git apply` again.** Keep backups of the database, quests, binaries and active opcode configuration together before installing. No running server was modified by publishing this branch.

```sh
git -c core.autocrlf=false clone --branch rof2-spirit-shrouds --single-branch https://github.com/nightstalker6669/EQEmu.git
cd EQEmu
git submodule update --init --recursive
```

## Current upstream foundation

All feature branches now share the same merged upstream base. `master` follows the verified current stable upstream; the common feature foundation also includes the protocol-development changes needed by achievements. See [the branch map](BRANCHES.md) and [exact provenance](integration.json).

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

## 3. Start the matched installation

Only after all required schema/content/quest groups succeed, start the server through its normal service manager. All world and zone processes must use the newly built cohort, cache and mapping. Check startup and perform representative native acceptance for each enabled feature. Preserve the installation receipt and matched rollback backup; source publication/checks do not prove deployment or native gameplay on this combined branch. The original release validation remains documented in its status guide.

## References

| Feature | Published input | Detailed prerequisites / policy |
| --- | --- | --- |
| housing | [`710ecb7`](https://github.com/nightstalker6669/eqemu-rof2-housing/tree/710ecb7530ff0a5ade837e887ad7bc2f84622d07) | [Guide](../../extensions/rof2/housing/docs/INSTALL.md) |
| fellowship | [`9382453`](https://github.com/nightstalker6669/eqemu-rof2-fellowship/tree/9382453264f4f84ac7e83a9ee5c9c9a1391c6017) | [Guide](../../extensions/rof2/fellowship/docs/INSTALL.md) |
| mercenaries | [`00b07c3`](https://github.com/nightstalker6669/eqemu-rof2-mercenaries/tree/00b07c381a738a21aff08867bf186e6c2bfaffb0) | [Guide](../../extensions/rof2/mercenaries/docs/INSTALL.md) |
| spirit-shrouds | [`e32ca15`](https://github.com/nightstalker6669/eqemu-rof2-spirit-shrouds/tree/e32ca155a67b6f4d12eef36de131c5810998517a) | [Guide](../../extensions/rof2/spirit-shrouds/docs/INSTALL.md) |

[Opcodes and structures](PROTOCOL.md) · [Integration manifest](integration.json) · [Original upstream README](UPSTREAM-README.md)


## Verification

See [integration.json](integration.json) for validation of the current source. Original release evidence is retained separately. No live server or database was changed during publication.
