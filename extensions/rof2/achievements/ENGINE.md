> **Current integrated fork:** use [the current branch installation guide](../../../docs/rof2/INSTALL.md). This document preserves the original release’s source baseline and evidence. Its source patches are already integrated and must not be reapplied here.

# Achievement engine patch series

Start with upstream [EQEmu](https://github.com/EQEmu/EQEmu) commit **`c9c8747ac949deddb140d196d241fa4e777500c7`**. Patch0001 is the production subset of [PR5119](https://github.com/EQEmu/EQEmu/pull/5119), by **fryguy503**, pinned at head **`c68d0074bbc35974b6413c208c4523dd7fd21954`**. Upstream attribution and source licensing remain applicable. This patch is not designed for arbitrary older forks or a housing-modified local checkout.

Apply the nine numbered patches in order from the EQEmu source directory:

```sh
git checkout c9c8747ac949deddb140d196d241fa4e777500c7
git apply /path/to/release/patches/0001-upstream-pr5119.patch
git apply /path/to/release/patches/0002-class-rewards-and-common-only.patch
git apply /path/to/release/patches/0003-council-destructible-and-ground-items.patch
git apply /path/to/release/patches/0004-unbound-component-completion-barrier.patch
git apply /path/to/release/patches/0005-shards-native-quest-support.patch
git apply /path/to/release/patches/0006-npc-bound-task-selector.patch
git apply /path/to/release/patches/0007-blightfire-cub-trap-permission.patch
git apply /path/to/release/patches/0008-moorwalker-runestaff-permission.patch
git apply /path/to/release/patches/0009-idle-zone-task-reload.patch
```

Patch0001 supplies definitions, state, progress, comparison, earned notification, reward selection, persistence, event hooks, repositories, commands, serializers and RoF2 mappings. Tests, test fixtures, example reward grants, the legacy ToB coverage report and its broad criteria repair SQL are excluded. The schema migrations remain included.

Patch0002 adds permanent-class reward eligibility, common-only claim authentication, cursor-persistence acknowledgement and two corrected skill IDs in the importer. Patch0003 supplies the destructible suffix/visibility and charged ground-item fixes needed by Crescent quest objects. Patch0004 prevents a partially authored definition from completing when only its mapped subset is satisfied. Patch0005 adds a lossless Lua shared-task instance identity and narrowly scoped NPC targeting permissions for the two Shards quest-item spells. The permissions require the exact item, spell shape, task activity, zone and eligible target; ordinary spell restrictions remain in effect. It includes no unrelated Spirit Shroud code. The matching [Shard’s Landing content package](content/shards-landing/README.md) supplies the tasks, focused quest hooks and installation guidance. Its provenance distinguishes native solo/single-member observations from additional isolated multiplayer checks, including Unspoken group ownership and the one-apparition limit.

Patch0006 adds `Client:TaskSelectorFromNPC(giver, task_ids)` for object and door scripts. It verifies a registered NPC giver within 60 units and finite positive integral task IDs before dispatching the native selector. Native level, repeatability, task-slot and request-cooldown checks remain authoritative. Actual Blightfire sign doors opened their native task lists and accepted a task on the existing local port; the full Blightfire content package remains a separate milestone. No opcode or packet-layout changes are introduced.

Patch0007 extends the restricted quest-item interaction helper to the original Slashclaw Cub Trap. It permits only item85753 casting spell9255 while task3040105 activity1 is active, in Blightfire Moors, against one of the seven original living, unowned cub types. The original spell shape and genuine item-cast context must match. General spell restrictions and the earlier Shards interactions remain unchanged. The matching [Welcome content package](content/blightfire-welcome/README.md) supplies its spell callback and task. No opcode or structure changes are introduced.

Patch0008 extends the same restricted helper to item85760 casting spell9181 during task3040208, in Blightfire Moors, against its four original living, unowned Moorwalker types. Each target requires its own active objective and the genuine item-cast context and spell shape. The matching [Champion content package](content/blightfire-champion/README.md) enforces the researched weakening threshold and distinct-target credit. No opcode or structure changes are introduced.

Patch0009 makes global task reloads reach idle zone processes. Their TaskManager caches persist before a zone boots; skipping those processes can make newly accepted tasks appear unknown and be discarded after zoning. Native reload refreshed all six processes, including five idle ones, and the cross-zone Mesa chain then completed. Only `world/zonelist.cpp` changes; no opcode or structure changes are introduced. See the complete [Mesa content package](content/mesa/README.md).

Build the patched server using the upstream instructions for your platform. Apply both upstream achievement schema migrations (`2026_07_28_achievement_content.sql`, `2026_07_28_achievement_state.sql`) and the follow-up `2026_10_27_achievement_reward_classes.sql` to the server database before starting the new binaries. The first two are registered with the pinned upstream database updater; the added class-mask migration must also be applied explicitly. Back up the database before upgrading. Use the release's curated historical content overlay separately; importing a current Live client catalogue does not make it May2013 content.

This offline package passed `git apply --check` and sequential application of all nine layers against the exact saved upstream changed-file preimages (plus the pinned unchanged `zone/zone.cpp` fetched for patch0003). A full clean-port Linux x86-64 build of `zone`/`world` passed for the first eight layers; patch0009 passed a dependency-aware `world` rebuild on that pinned source with GCC12.2, Release mode and upstream-default precompiled headers. Clean-port native validation remains pending. Native observations from the existing running local port validate that port, not this reconstructed build. `engine-provenance.json` records included/excluded files and patch digests.

The [protocol guide](protocol/README.md) provides mappings and packet layouts, including structures whose lengths depend on the catalogue. No client binaries/assets, raw client resource catalogues, captures, private character data, test files or housing/fellowship payloads are part of this package.




## Roost bound-destructible integration

The Roost milestone integrates twelve production source files directly, adding trusted collider ownership and actor serialization plus the reviewed lootless-destructible ruins lifecycle. No additional opcode/schema patch is required. Rebuild all Map/BVH-header consumers; follow the [Roost content guide](content/roost/README.md). Archived patch commands are not additional installation steps for this checkout. Exact original Live server lifecycle remains unproved.

## Steppes shared-task and scene integration

The integrated source includes full-width shared reward occurrence identity, guarded whole-party admission and durable completion archives with retry-before-removal, item-cast custody fields for Lua scenes, Lua door/member APIs and recurring random-ground height correction. Rebuild/deploy the matching world and zone consumers; the corresponding combined Linux13target build passed. Achievement-only full linking/native rerun is not claimed.

Before starting patched services, apply `utils/sql/git/2026_10_30_shared_task_reward_identity.sql` and `utils/sql/git/2026_10_30_shared_task_admission_policies.sql` to the server database. The complete [Steppes installer](content/steppes/README.md) also includes guarded schema prerequisites, all content/activation and the researched Gnoll late-join policy. These are direct source changes on this branch; no extra archived patch applies. Existing opcode mappings and packet structures are retained; see [Steppes protocol scope](content/steppes/PROTOCOL.md).

## NPC buff state saves

NPC state serialization uses the engine's `IsValidSpell` predicate. Empty slots in this build use `SPELL_UNKNOWN` (`-1`); filtering only `0` and `65535` incorrectly serialized their inactive fields and could produce invalid text in the state table. The change does not alter the saved buff layout or require a schema migration. A native empty-buff Roost shutdown saved 300 state rows without the earlier serialization error. Active-buff restoration was not exercised in that sample.

## Quest-created ground-item loot

Item objects created through `eq.create_ground_object` can now satisfy native loot objectives after successful pickup. A server-only origin flag distinguishes these one-shot quest objects from player drops; it does not enable recurring respawn behavior. Existing pickup vetoes, duplicate-lore handling and inventory-placement checks remain authoritative. Player-dropped items stay ineligible for loot credit.

Rebuild the Object-header consumers through the normal full server build. All configured Linux targets passed, followed by native RoF2 quest-ground pickup, hand-in and persistent achievement/reward verification. The Sunderock content package remains a separate milestone. No new opcode, wire structure or database migration is introduced; native Windows execution remains unverified.
