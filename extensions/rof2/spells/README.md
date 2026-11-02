# RoF2 spells

Database and quest content repairs for class spells through Rain of Fear, targeting the May 10, 2013 RoF2 client. This branch contains the spell work completed so far. It does **not** establish exact Live implementation of every spell.

The branch inherits `rof2-shared-core` commit `17d63f7d512711e911787e630f4533ee14a1e785` on upstream `develop` commit `c9c8747ac949deddb140d196d241fa4e777500c7`. The spell layer adds server-side merchant language eligibility, spell content, original rune exchange scripts and ground-target aura placement. Public files exclude client assets, private character data, credentials, logs, test fixtures and server binaries.

## Included content

| Stage | Content |
| --- | --- |
| `001-exigent-items` | Three missing summoned Exigent Servant/Minion items: 64981, 64983 and 64984 |
| `002-rank1-vendors` | 574 distinct RoF rank I spells/tomes, 599 merchant rows across eight existing Shard's Landing vendors, and Magus Burlshin's Crystal Circle Builders faction binding |
| `003-pets-and-auras` | **Optional estimated reconstruction:** 202 missing pet template names, 3,084 pet/focus variants and corresponding unspawned NPC templates, 30 AI headers/entries, and 258 aura definitions |
| `005-voa-language` | 182 sourced Argath stock entries and Alaran language gates on 159 existing entries, covering 336 distinct rank I spells across eight merchants |
| `004-kerk-motes` | Kerkaz's script, ten physical Dreadmote/Terrormote delivery tasks, class-filtered native reward choices covering 1,108 distinct rank II/III spells/tomes, and two missing scroll items |

| `006-ordered-runes` | Original ordered exchanges for 284 SoF and 57 Underfoot rank III rewards, four quest/module files and the missing Drifting Haze III scroll |

Stage 003 includes measured historical anchors where available, but all reconstructed pet NPCs contain estimated secondary statistics. Aura ranges, timings and visual templates also include estimates. It is excluded by the installer's default selection. The [pet model](provenance/pet-model.json) and [aura model](provenance/aura-model.json) identify the scope and sources. These estimates must not be presented as recovered Live balance.

Kerkaz uses the original acquisition route: say `dreadmote` or `terrormote`, accept the matching task, deliver the physical mote, then choose a reward in the native window. Documented two- and three-scroll bundles remain one choice. The original mote can be selected back. There is no alternate-currency exchange merchant or added trade-down conversion. Hail/description text is paraphrased; this is not an exact dialogue transcript recovery.

SoF Kalim`Kar and Underfoot Dwekvard use direct physical rune hand-ins. Each exchange gives the first unlearned spell or discipline in the documented class/rune order and returns unused items. They do not use selectable task rewards. Tjudawos’ Chant of Flame III remains unresolved: the guide lists it, but no scroll identity was recovered; no item ID or replacement reward was invented. See [ordered rune provenance](provenance/ordered-runes.json). This stage does not implement the separate faction vendors, Underfoot mission rewards or rank II gem drops.

Ground-target persistent auras are created once at the caster's selected ring position. Range, minimum range, finite coordinates and detrimental line of sight are checked before casting and again before creation. Ordinary caster auras and nonpersistent area spells retain their existing placement. Native gameplay acceptance remains pending.

## Engine ownership and dependency

The generic task reward catalogue, native selection window, permanent-class filtering and persisted delivery ledger are now inherited from [`rof2-shared-core` commit `17d63f7d512711e911787e630f4533ee14a1e785`](https://github.com/nightstalker6669/EQEmu/tree/17d63f7d512711e911787e630f4533ee14a1e785). Achievement progress, criteria, providers and quest content are not required or enabled by this branch.

The shared engine is already integrated into the normal source directories. Build this branch; do not merge an achievement branch or reapply historical source overlays. Run its matching updater through schema version 9336 and review the three original supplemental class/shared-task SQL files in [shared reward installation](../../../docs/rof2/SHARED-REWARDS.md). Migration IDs 9331–9334 and their original SQL are preserved. The compatibility migrations retain empty achievement schema scaffolding so a later achievement installation remains compatible; those empty tables do not activate achievements. The original migration 9335 is also retained. Spells adds content migration 9336 for merchant language requirements without renaming earlier registrations.

Stage 004 still requires the matching reward-capable binary; the content installer cannot attest which executable is running. A table-only installation on a stock `develop` binary is insufficient.

See [installation and rollback](docs/INSTALL.md), [research and limitations](docs/RESEARCH.md), and [verification scope](docs/VERIFICATION.md).

## Layout

- `data/`: canonical insert/update payloads; no full database dump.
- `quests/`: Kerkaz, the two ordered rune NPC scripts and their shared Lua catalogue/module.
- `tools/install.py`: read-only planning by default, guarded transactional application, targeted backup and preserving quest-file installation.
- `sql/`: equivalent SQL and original content rollback scripts for review; the installer performs preflight that raw SQL lacks.
- `provenance/`: game-content sources, estimate models and the complete class/tier reward catalogue.

Upstream [GPLv3 licensing](../../../LICENSE.md) applies to the code. Source links identify external research; no external article archive or raw client resource catalogue is bundled.
