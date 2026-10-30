> **Current integrated fork:** use [the current branch installation guide](../../../docs/rof2/INSTALL.md). This document preserves the original release’s source baseline and evidence. Its source patches are already integrated and must not be reapplied here.

# EQEmu RoF2 achievements

A source release of the **RoF2 achievement engine, opcode mappings, packet layouts, reward selection support and reviewed Crescent Reach, Blightfire Moors, Stone Hive and Shard’s Landing quest content**. It targets the May10,2013 RoF2 client and a pinned EQEmu source baseline.

The release includes the engine/protocol and Crescent Reach, Blightfire Moors, Stone Hive and Shard’s Landing content milestones. It does **not** claim that every achievement or quest through Rain of Fear is playable. The content packages contain twenty-six reviewed achievement definitions. The wider historical research catalogue admits 254 identities; unfinished content remains outside this release.

## What is included

- Achievement definitions, primary state, progress updates, comparisons and completion notifications.
- Persistent achievement/task reward selection, permanent-class filtering and common-only reward claiming.
- Event hooks and quest APIs for authoring criteria.
- **All 12 achievement/reward opcode mappings**, a standalone packet-prefix header, variable layout documentation and canonical serializer source excerpts.
- Nine ordered production patches, provenance and the upstream license.
- [Crescent Reach content](content/crescent/README.md): scoped quest and catalogue SQL, NPC scripts, acquisition/reward dependencies, installation checks and labelled defaults. Its guide distinguishes native gameplay evidence from source review.
- [Shard’s Landing content](content/shards-landing/README.md): nine tasks covering the Partisan arc and portal prerequisite, focused quest-hook patches, guarded upgrades and source provenance. All eight configured Partisan components completed in native solo or single-member shared-task checks. Additional two-member acceptance, kill deduplication, creator-absence and foreign-instance checks passed; Unspoken group ownership and the one-apparition limit passed representative checks too. The guide records QA setup and exact coverage boundaries.

- [Blightfire Moors content](content/blightfire-moors/README.md): fourteen quests, 44 objectives and three achievement groups. All 14 quests completed through native gameplay; their components and all 3 parents persisted across logout/login. Includes real job boards, original acquisition routes, the Rotwood cycle, guarded installation and labelled historical reconstructions.

- [Bixie and Makesn quests](content/blightfire-bixies/README.md): twelve quests, 58 objectives and two achievement groups. Native quest completions, reward choices and all twelve component bits persisted across logout/login. Includes cocoons, buckets, Silo and living rescue scenes, plus guarded installation and explicit reconstruction limits.

- [Battling Blightfire quests](content/blightfire-spirits/README.md): seven quests and 24 objectives, including wand attunement, Lore Siphon, Nokk fire and skull rituals, and confirmed quest rewards. See its guide for native gameplay evidence and historical limits.

- [Welcome to the Moors quests](content/blightfire-welcome/README.md): eight quests and 39 objectives, including the Golden Pick mining chain, cub capture, original corpse drops and the ratwork rescue. Native quest rewards and saved achievement completion are covered; parent gear is supplied by the required reward upgrade below.

- [Champion of the Moors quests](content/blightfire-champion/README.md): eight quests and 23 objectives, including the original bone-pile loot, witch hand-ins, Defan's floor-to-roof encounter, torch/signal interactions and all four Runestaff targets. The guide records native completion, persistence and the single GM crest used for turn-in validation.

- [Stone Hive quests and rewards](content/stonehive/README.md): sixteen quests, 33 objectives and four achievement groups, with native completion and fresh-login persistence. Includes scout ground pickups, original botany/cooking acquisition, Warwing and Queen Pelzia, plus four class-filtered parent item reward sets. The guide labels encounter reconstructions and historical reward uncertainties.

- [Goru`kar Mesa quests and rewards](content/mesa/README.md): nineteen native-completed quests, original resource interactions, Madu’s five-quest chain, Pranks rewards and fresh-login persistence. The twentieth capture quest stays disabled; Welcome’s missing head rewards and Berserker Pranks remain unresolved. Includes Linux, native Windows and Akk-stack upgrade directions.

- [Mesa armor, gifts and encounters](content/mesa-remaining/README.md): 36 additional quests,277 activities,27 recipes and four historical achievement definitions. Includes class armor, paid gifts, ordered Kathryn defense, a real corpse note and Simu escort, plus four researched parent reward sets. The coverage report distinguishes ordinary gameplay, private scene assignments and reconstructed route/balance settings.

- [Blackfeather Roost quests, acquisition and nests](content/roost/README.md):15 quests,54 activities,five historical definitions,three confirmed parent jewellery sets and bound nest combat/lifecycle. Two armour reward choice sets remain unresolved.

- [Steppes quests and shared-task persistence](content/steppes/README.md):13quest routes,59activities,three historical definitions and four reward sets. Twelve quests and the complete seven-component Conquering reward path passed native checks; the three-player crypt, multiplayer and recurring pickup limits remain explicit.

**Reward coverage:** Install the complete [Blightfire parent gear upgrade](content/blightfire-parent-rewards/README.md) after the five Blightfire quest packages. It adds the eight quest-family parent reward sets, including class-appropriate armor/accessories and shield/weapon bundles. The separate Conqueror of Blightfire meta-achievement bundle remains unconfigured while its original selection rules are researched. Stone Hive’s four parent reward sets are included in its package. Historical and class-coverage limits are documented in each guide.

No tests, captures, client assets or private runtime records are included.

## Install

Read [ENGINE.md](ENGINE.md) for the exact source commit, patch order, schema migrations and build instructions. This is a source upgrade requiring a server rebuild; it is not a binary drop-in or a complete content/database replacement.

On this integrated branch, use [the current installation guide](../../../docs/rof2/INSTALL.md) and skip all source patch commands. The list below is archived standalone-release provenance only; those changes are already present. The original standalone source baseline was `c9c8747ac949deddb140d196d241fa4e777500c7`:

1. `patches/0001-upstream-pr5119.patch` — achievement/reward engine and RoF2 protocol.
2. `patches/0002-class-rewards-and-common-only.patch` — class eligibility, common-only claims and persistence corrections.
3. `patches/0003-council-destructible-and-ground-items.patch` — quest-object rendering and ground-item charges.
4. `patches/0004-unbound-component-completion-barrier.patch` — prevent partially mapped definitions from falsely completing.
5. `patches/0005-shards-native-quest-support.patch` — shared-task identity and restricted quest-item targeting support.
6. `patches/0006-npc-bound-task-selector.patch` — native task offers from an object or door through a nearby NPC giver.
7. `patches/0007-blightfire-cub-trap-permission.patch` — restricted targeting for the original Slashclaw Cub Trap quest interaction.
8. `patches/0008-moorwalker-runestaff-permission.patch` — restricted targeting for the original Moorwalker Runestaff quest interaction.
9. `patches/0009-idle-zone-task-reload.patch` — refresh task definitions in idle zone processes, preventing newly accepted quests from disappearing when those processes later host a zone.

After building the engine and applying its schema migrations, follow the [Crescent](content/crescent/README.md), [Blightfire](content/blightfire-moors/README.md) and [Shard’s Landing](content/shards-landing/README.md) content guides for the ordered SQL and quest files. Then install the [Bixie and Makesn package](content/blightfire-bixies/README.md) after Blightfire Moors. Install [Battling Blightfire](content/blightfire-spirits/README.md) after Crescent Reach. Install [Welcome to the Moors](content/blightfire-welcome/README.md) after Crescent Reach and Blightfire Moors. Install [Champion of the Moors](content/blightfire-champion/README.md) after Crescent Reach and Blightfire Moors, with patch0008. Install the complete [Stone Hive package](content/stonehive/README.md) with all four required SQL files and its script set after the engine/schema foundation. Its included delivery adapter preserves earlier Cook behavior. Then apply the required [Blightfire parent gear upgrade](content/blightfire-parent-rewards/README.md) to complete the eight family rewards. Then install the complete [Mesa package](content/mesa/README.md), including patch0009 and all four SQL files. Finally install the complete [Mesa armor, gifts and encounters extension](content/mesa-remaining/README.md), including every required SQL file and callback. Then install the [Roost milestone](content/roost/README.md), its five base imports and separately reviewed known-scope activation, preserving the two unresolved armour reward gaps. Then install the complete [Steppes milestone](content/steppes/README.md) after rebuilding its engine and applying both new schema migrations. Its installer includes every SQL/script file and activation. Their guarded preflights detect conflicting custom content before installation.

Apply the schema migrations before launching patched binaries. See [the protocol reference](protocol/README.md) for the opcode table and structures. [engine-provenance.json](engine-provenance.json) lists exact file scope and patch digests.

## Verification

Each layer passed sequential patch-application checks against the pinned source preimages. The current running local port has native achievement/reward observations. The first eight layers built `zone` and `world` from clean pinned source; patch0009 then passed sequential application and a dependency-aware `world` rebuild on that same Linux x86-64/GCC12.2 Release configuration with default precompiled headers. Native gameplay on this reconstructed clean-port build remains pending; existing local-port observations are a separate claim.

## Attribution

The engine originates in [EQEmu PR5119](https://github.com/EQEmu/EQEmu/pull/5119) by **fryguy503**, pinned at `c68d0074bbc35974b6413c208c4523dd7fd21954`. Follow-up changes were prepared during this project's RoF2 achievement restoration. The upstream EQEmu source is licensed under [GPLv3](LICENSE); retained upstream notices and license terms apply. See [ATTRIBUTION.md](ATTRIBUTION.md).




