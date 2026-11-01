# RoF2 spells

Database and quest content repairs for class spells through Rain of Fear, targeting the May 10, 2013 RoF2 client. This branch contains the spell work completed so far. It does **not** establish exact Live implementation of every spell.

The branch inherits `rof2-shared-core` commit `fc88a0ba79a05e437e7b61af4e38ead32131de0f` on upstream `develop` commit `c9c8747ac949deddb140d196d241fa4e777500c7`. The spell layer adds no C++ spell engine changes, client assets, private character data, credentials, logs, test fixtures or server binaries.

## Included content

| Stage | Content |
| --- | --- |
| `001-exigent-items` | Three missing summoned Exigent Servant/Minion items: 64981, 64983 and 64984 |
| `002-rank1-vendors` | 574 distinct RoF rank I spells/tomes, 599 merchant rows across eight existing Shard's Landing vendors, and Magus Burlshin's Crystal Circle Builders faction binding |
| `003-pets-and-auras` | **Optional estimated reconstruction:** 202 missing pet template names, 3,084 pet/focus variants and corresponding unspawned NPC templates, 30 AI headers/entries, and 258 aura definitions |
| `004-kerk-motes` | Kerkaz's script, ten physical Dreadmote/Terrormote delivery tasks, class-filtered native reward choices covering 1,108 distinct rank II/III spells/tomes, and two missing scroll items |

Stage 003 includes measured historical anchors where available, but all reconstructed pet NPCs contain estimated secondary statistics. Aura ranges, timings and visual templates also include estimates. It is excluded by the installer's default selection. The [pet model](provenance/pet-model.json) and [aura model](provenance/aura-model.json) identify the scope and sources. These estimates must not be presented as recovered Live balance.

Kerkaz uses the original acquisition route: say `dreadmote` or `terrormote`, accept the matching task, deliver the physical mote, then choose a reward in the native window. Documented two- and three-scroll bundles remain one choice. The original mote can be selected back. There is no alternate-currency exchange merchant or added trade-down conversion. Hail/description text is paraphrased; this is not an exact dialogue transcript recovery.

## Engine ownership and dependency

The generic task reward catalogue, native selection window, permanent-class filtering and persisted delivery ledger are now inherited from [`rof2-shared-core` commit `fc88a0ba79a05e437e7b61af4e38ead32131de0f`](https://github.com/nightstalker6669/EQEmu/tree/fc88a0ba79a05e437e7b61af4e38ead32131de0f). Achievement progress, criteria, providers and quest content are not required or enabled by this branch.

The shared engine is already integrated into the normal source directories. Build this branch; do not merge an achievement branch or reapply historical source overlays. Run its matching updater through schema version 9334 and review the three original supplemental class/shared-task SQL files in [shared reward installation](../../../docs/rof2/SHARED-REWARDS.md). Migration IDs 9331–9334 and their original SQL are preserved. The compatibility migrations retain empty achievement schema scaffolding so a later achievement installation remains compatible; those empty tables do not activate achievements. No duplicate or renamed migration registrations are added by spells.

Stage 004 still requires the matching reward-capable binary; the content installer cannot attest which executable is running. A table-only installation on a stock `develop` binary is insufficient.

See [installation and rollback](docs/INSTALL.md), [research and limitations](docs/RESEARCH.md), and [verification scope](docs/VERIFICATION.md).

## Layout

- `data/`: canonical insert/update payloads; no full database dump.
- `quests/`: the new Kerkaz NPC script only.
- `tools/install.py`: read-only planning by default, guarded transactional application, targeted backup and preserving quest-file installation.
- `sql/`: equivalent SQL and original content rollback scripts for review; the installer performs preflight that raw SQL lacks.
- `provenance/`: game-content sources, estimate models and the complete class/tier reward catalogue.

Upstream [GPLv3 licensing](../../../LICENSE.md) applies to the code. Source links identify external research; no external article archive or raw client resource catalogue is bundled.
