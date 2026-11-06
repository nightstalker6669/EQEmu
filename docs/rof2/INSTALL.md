# Install the shared core

## Current upstream foundation

All feature branches now start directly from upstream `develop` commit **`c9c8747ac949deddb140d196d241fa4e777500c7`**, with no master merge in their new history. `master` remains the separate stable upstream branch. See [the branch map](BRANCHES.md), [clean comparisons and clone updates](COMPARE.md), and [exact provenance](integration.json).

Before applying feature migrations to an older database, build the matching current `world` binary through the normal upstream/vcpkg workflow and run its database updater from the configured server directory (`world database:updates`, or `world.exe database:updates` on Windows). Back up first and keep game services stopped. Review and complete the updater before the feature installation below. The housing migration ledger is not a replacement for upstream schema upgrades.

Current upstream uses vcpkg, CMake 3.20+, and a C++20 compiler. Initialize submodules before configuring. For native Windows, use the current upstream Visual Studio/vcpkg toolchain; for Akk-stack, use a build container with those dependencies and the correct mounts. System libraries alone are not a substitute for the current dependency manifest.

Build through the normal upstream workflow. The core alone does not activate the feature session hooks. Its additive `utils/sql/shared_core/001_inventory_sessions.sql` preserves the original durable table names; follow the [core boundaries](../../common/shared_core/README.md) before provisioning it. Feature branches already inherit this code and apply their required schema through their documented migration path.

[Branch map](BRANCHES.md) · [Provenance](integration.json)

The shared core now also provides native task selectable rewards, shared occurrence ownership, carried Collect high-water tracking and distinct task-completion reward gates. Review [shared reward installation and migration compatibility](SHARED-REWARDS.md), including the original supplemental class/identity/admission SQL, before using those capabilities. This source targets binary schema version **9338**.

Schema 9335 adds only the empty `shared_task_average_level_policies` content table and its schema guard. No mission/policy/content rows are activated by the core. Keep the exact 9335 registration when combining this core with later feature migrations. [Transport and shared-task APIs](transport-passenger-state.md) describe the generic Lua accessors and optional average-level admission.

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
