> **Current integrated fork:** use [the current branch installation guide](../../../docs/rof2/INSTALL.md). This document preserves the original release’s source baseline and evidence. Its source patches are already integrated and must not be reapplied here.

# EQEmu RoF2 achievements

A source release of the **RoF2 achievement engine, opcode mappings, packet layouts, reward selection support and reviewed Hero’s Journey and Shard’s Landing quest content**. It targets the May10,2013 RoF2 client and a pinned EQEmu source baseline.

The release includes the engine/protocol and Crescent Reach, Blightfire Moors, Stone Hive and Shard’s Landing content milestones. It does **not** claim that every achievement or quest through Rain of Fear is playable. The packages below document their reviewed playable scope. The wider historical research catalogue admits 254 identities; unfinished content remains outside this release.

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

- [Steppes quests and shared-task persistence](content/steppes/README.md):13quest routes,59activities,three historical definitions and four reward sets. All thirteen quests and all three parent reward paths passed native checks, including the three-player crypt, shared rejoin and a completed-member reconnect. Remaining late-join, recurring pickup and platform limits are explicit.

- [Sunderock quest and item-reward milestone](content/sunderock/README.md): 15 native-completed routes, 56 objectives and four parent item reward paths. Includes the full guarded content installer, provenance and protocol references. Numeric faction rewards and Mine raid access remain unfinished.

- [Short Sharp Sting](content/short-sharp-sting/README.md): four native-completed Stone Hive quests, 18 objectives, journal container/recipe support, faction and copy encounters, quest rewards and class-filtered parent gloves. Includes a complete guarded installer for Linux, native Windows and Akk-stack; historical defaults and QA limits are explicit. Native Windows execution remains unverified.

- [Vergalid Mines quests and rewards](content/vergalid/README.md): 17 native-completed quests, 56 objectives, four parent reward paths and seven restored actor types. Includes the chest, bait scenes, charged-shard/statue passage, shared orb encounter, native death-state corrections and a complete guarded installer. Quest/reward/key persistence passed fresh login. Historical defaults, head-variant inference and remaining multiplayer/Windows limits are explicit.

- [Buried Sea islands and Jonas Dagmire’s hand](content/buriedsea/README.md): 39 island quests,180 objectives,seven armor reward sets and all27bones; seven native consumed-compound recipes produce the final hand. Includes Mer’s real three-member shared mission, original ferry/rowboat transport,40Doubloons per member,Halon’s complete stock,and a guarded whole-package installer plus a RoF2 ship-model-list repair. Native completion and combined login persistence passed within the documented scope; historical and platform limits remain explicit.

- [Dragonscale Hills quests](content/dragonscale/README.md): all 15 quests and 43 objectives for the strike force, challenges and Ladies of the Light families. Includes original givers, farm gears, physical banner planting, crystal/canvas pickups, Lelluran and claimant-owned rescue guards. Native completion, all three achievement families and fresh-login persistence passed; researched values and labelled defaults are documented.

