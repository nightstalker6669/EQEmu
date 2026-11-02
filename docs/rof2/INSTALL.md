# Install `rof2-achievements`

This checkout already contains the native source integration. **Do not run `apply_overlay.py` or `git apply` again.** Keep backups of the database, quests, binaries and active opcode configuration together before installing. No running server was modified by publishing this branch.

```sh
git -c core.autocrlf=false clone --branch rof2-achievements --single-branch https://github.com/nightstalker6669/EQEmu.git
cd EQEmu
git submodule update --init --recursive
```

## Current upstream foundation

All feature branches now start directly from upstream `develop` commit **`c9c8747ac949deddb140d196d241fa4e777500c7`**, with no master merge in their new history. `master` remains the separate stable upstream branch. See [the branch map](BRANCHES.md), [clean comparisons and clone updates](COMPARE.md), and [exact provenance](integration.json).

Before applying feature migrations to an older database, build the matching current `world` binary through the normal upstream/vcpkg workflow and run its database updater from the configured server directory (`world database:updates`, or `world.exe database:updates` on Windows). Back up first and keep game services stopped. Review and complete the updater before the feature installation below. The housing migration ledger is not a replacement for upstream schema upgrades.

Current upstream uses vcpkg, CMake 3.20+, and a C++20 compiler. Initialize submodules before configuring. For native Windows, use the current upstream Visual Studio/vcpkg toolchain; for Akk-stack, use a build container with those dependencies and the correct mounts. System libraries alone are not a substitute for the current dependency manifest.

## Engine, database and quests

The nine patches in the [engine guide](../../extensions/rof2/achievements/ENGINE.md) are already applied. Follow its **database migration and deployment** directions, skipping the source patch commands. Apply `2026_07_28_achievement_content.sql`, `2026_07_28_achievement_state.sql`, then `2026_10_27_achievement_reward_classes.sql` from the integrated source. The first two are registered with the pinned updater; inspect existing schema/history before any replay.

Install **all reviewed content** in the order given in the [release installation section](../../extensions/rof2/achievements/README.md#install), running each package's guarded preflights and quest installer and importing every required SQL file. This includes Crescent, Shard’s Landing, all five Blightfire quest packages, Stone Hive, Blightfire parent rewards, Mesa, then the complete [Mesa armor/gifts/encounters extension](../../extensions/rof2/achievements/content/mesa-remaining/README.md). Then install the [Roost milestone](../../extensions/rof2/achievements/content/roost/README.md), including all five base SQL imports, preserving hooks and the separately reviewed known-scope activation. Ascending and Raider armour rewards remain unconfigured. Rebuild the integrated Roost engine source; no new schema or opcode migration is needed. Keep unknown conflicting quest scripts for a deliberate merge. Do not import research catalogues as playable content.

Build and deploy matching world/zone binaries and `utils/patches/patch_RoF2.conf` through your normal Linux, native Windows or Akk-stack workflow. The [Mesa guide](../../extensions/rof2/achievements/content/mesa/README.md) includes native Windows and Akk-stack examples. Achievements shares the same upstream foundation as the other feature branches. Later achievement-only content commits are published here; consult the combined branch’s manifest before assuming it includes those later milestones.

## References

[Achievement engine and provenance](../../extensions/rof2/achievements/ENGINE.md)

[Opcodes and structures](PROTOCOL.md) · [Integration manifest](integration.json) · [Original upstream README](UPSTREAM-README.md)

## Verification

See [integration.json](integration.json) for validation of the current source. Original release evidence is retained separately. No live server or database was changed during publication.

Steppes adds two required schema migrations and a complete guarded content installer. Follow the [Steppes engine section](../../extensions/rof2/achievements/ENGINE.md#steppes-shared-task-and-scene-integration) and [content guide](../../extensions/rof2/achievements/content/steppes/README.md). Back up and stop writers; deploy the matching rebuilt engine before players reconnect.

## Short Sharp Sting content upgrade

After the existing Stone Hive foundation, install the complete [Short Sharp Sting package](../../extensions/rof2/achievements/content/short-sharp-sting/README.md). Use its guarded installer for all five SQL phases and all quest callbacks together. Rebuild and deploy this branch’s matching engine first; the journal requires the recovered RoF2 container decoder. The package includes Linux, native Windows and Akk-stack directions and targeted backups. Windows execution remains unverified.

## Vergalid Mines content upgrade

Install the complete [Vergalid package](../../extensions/rof2/achievements/content/vergalid/README.md), including all four SQL phases, actor/acquisition dependencies and preserving quest callbacks. Its one installer supports Linux, native Windows and Akk-stack workflows with private database command configuration and full backups. Native Windows execution remains unverified. Use this branch's matching engine, including the restricted original shard item-cast event support; no additional wire opcode is introduced. All 17 routes and four parent reward paths passed native checks and fresh-login persistence on the matching combined runtime. Consult the package coverage report for reconstruction and multiplayer limits.


## Carried collection and progression-dependent task rewards

The shared core supplies native carried-inventory Collect objectives and per-occurrence task reward choices gated by distinct quest completions. Use its matching core or feature binary and updater migrations **9333 (state)**, **9334 (content)** and **9335 (average-level content policy)**. These task capabilities do not require the achievement engine. Keep state/content database routing intact. Existing static rewards retain their existing policy. See [the progression guide](../../extensions/rof2/achievements/protocol/task-reward-progression.md) for definitions, delivery safeguards and validation scope. The extension reuses the documented RoF2 reward opcodes and structures; it adds no mapping or wire layout. The complete [Buried Sea content package](../../extensions/rof2/achievements/content/buriedsea/README.md) is included and installs through its whole-package planner. Its native milestone passed on the local schema 9336 runtime; that evidence is separate from native acceptance of the newly adopted schema 9338 engine.

## Shared reward dependency

This source adopts `rof2-shared-core` commit `b3f795ea642d810d0af75dca39d6dd1f28826385`, including the generic schema9336–9338 engine. Build this feature branch directly; no second core overlay is needed. Follow [shared reward schema compatibility](SHARED-REWARDS.md). Published migration9331-9334 identities and supplemental SQL names remain unchanged. Achievement progress/providers/content remain feature-owned.

Shared schema 9335 retains `2026_11_01_shared_task_average_levels.sql` exactly and supplies an empty optional average-level policy table. Generic [transport and owned shared-task instance APIs](transport-passenger-state.md) come from the shared core; no Buried Sea content is activated by this dependency update.

## Schema 9336–9338


Build the matching source before running the normal database updater. Keep the original migration IDs and SQL; do not renumber these changes when combining feature branches.

| Version | Database | Change |
| --- | --- | --- |
| 9336 | Content | Add merchant language requirements; defaults retain unrestricted existing stock. |
| 9337 | Content | Add the empty `task_spell_exchange_policies` table for explicitly registered personal exchanges. |
| 9338 | State | Pin each pending task reward's learned-spell policy, default `0` for legacy occurrences, and add the durable reward display ID allocator. |

Core migrations add schema only. Feature installers own the actual quest, merchant, item and policy rows. With separate content and character databases, configure both destinations before running the matching `world` updater. It routes each migration to the content or state database using the manifest flag.

Back up both databases and the allocator high-water mark before upgrading. Stop game services and drain old client reward sessions before the first allocator-enabled deployment. Use a coordinated client restart for the transition from older, reusable display IDs; native stale-cache transition acceptance remains pending.

**Never reset or decrease `reward_selection_wire_allocator.last_id`.** It is server state, not character data, and must stay out of character copy/reset and content rollback. Before restoring an older database snapshot, preserve the current largest issued high-water mark. Restore that value or a larger one before starting any server that can display rewards. Restoring a lower value can bind an old client response to a later choice. If the largest issued value cannot be established after data loss, keep reward displays disabled until a safe high-water mark has been recovered; do not initialize the counter to zero. Ordinary migration reruns retain the existing row and counter.

[Engine contracts, validation and native limits](SPELL-EXCHANGE-ENGINE.md) describe the new policies, display IDs and shutdown cleanup. Offline compilation and fixtures do not establish a completed native spell exchange or reconnect regression.

The Buried Sea milestone covers all 39 island quests, 27 bone rewards, seven Cleric armor choices, a real three-member Mer occurrence with one physical voyage and three payouts, seven hand recipes and one combined relog. It does not claim every armor variant, native Windows gameplay or the entire May 2013 achievement catalogue. The new engine is source/build adoption only until a coordinated deployment and native exchange/reconnect/shared-task regression pass.

This achievement consumer source independently linked `world` and `zone` with GCC12/C++20 and normal CMake/PCH settings in an isolated clean serial build. That build preserves the full published Buried Sea package. The build did not replace the running server; native regression acceptance for the new engine remains pending.
