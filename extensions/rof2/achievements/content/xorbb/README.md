# Xorbb quests and achievements for RoF2

**Status: complete source package for review; content is inactive and native acceptance is pending.** Migration 9345 composes the earlier 9344 Xorbb work with the eleven mercenary quests. It has not been registered or applied to the running database.

The combined package contains fourteen quest definitions and 120 objective source paths: 47 from the earlier partisan component and 73 from the mercenary component. It includes scripts, NPC profiles and caster AI, corpse drops, garden pickups, itemless crystal interactions, the Hollowflesh recipe, scenes and fixed rewards. These are required components of one package.

## What is included

| Part | Included work |
|---|---|
| Quests and progression | Requests, objectives, native deliveries, run tracking and persistence guards |
| NPCs | The prior 21-profile component, 136 ordinary profiles, 24 caster bindings and five owned scene definitions |
| Gathering and crafting | Conversion of 26 existing garden objects, three crystal clicks per color, and the original coffin recipe |
| Scenes | Lurr's eyes and capture, Grolak's lightning demonstration, Gronk's duck and three attackers |
| Rewards | Fixed reward definitions with researched values and labelled configurable defaults |
| Protocol documentation | **Existing configured opcodes and packet structures**, available in [protocol](protocol/) |

The migration has one outer transaction. It includes the earlier component once when absent; an existing component must match its owned postimages and namespace counts. Partial or different installations refuse. The NPC owner's joint component merges stat and caster changes before checking their final values. Unsupported split-database layouts refuse because saved world-container contents must be checked in the authoritative runtime database.

## Prepare a review folder

From this package directory, run the staging tool against your server's quest folder. It creates merged quest files, original-file snapshots and an installation plan in a new folder. It performs no SQL or active server-file writes. This source edition has no apply mode.

Linux, or an akk-stack host using its mounted quest folder:

```bash
python3 prepare_install.py --quests-dir /path/to/server/quests --stage-dir /path/to/xorbb-review
```

Native Windows:

```powershell
py -3 prepare_install.py --quests-dir "C:\EQEmu\server\quests" --stage-dir "C:\EQEmu\xorbb-review"
```

Open `INSTALL-PLAN.private.json` in the review folder. A conflict requires a real handler/module merge before installation. The stage preserves original handlers and refuses ambiguous numeric, named or Perl NPC callbacks and duplicate physical hand-in ownership. Use a new folder for each distinct plan.

Publication, coordinated installation and controlled activation come before the grouped native check. See [REQUIRED-COMPONENTS.json](REQUIRED-COMPONENTS.json) for current status and [INSTALL-CONTRACT.json](INSTALL-CONTRACT.json) for the engine and database boundaries. The local Build owner has deployed the required 64-entry ground loader and owned itemless-object interface; that establishes capability, not native scene or quest acceptance on another server.

## Opcodes and structures

[configured-opcodes.json](protocol/configured-opcodes.json) records the existing RoF2 object and task mappings. [rof2_xorbb_wire.h](protocol/rof2_xorbb_wire.h) documents click, container-action, known object-spawn fields and task-completion layouts. This quest batch introduces no new wire opcode or client executable change. Server ownership tokens and Lua `object_id` are separate from wire fields. The earlier [door/incline and item-verification structures](prerequisite9344/protocol/) are included as well.

## Historical fidelity

The cutoff is May 31, 2013. Period reports, current mutable guides, supplied maps, static client assets and implementation defaults are distinguished in [provenance](provenance/). Unknown XP uses the approved configurable PEQ-style default. Balance, missing encounter values, selected generic caster spellbooks and reconstructed crystal variants/layout/sharing retain explicit default labels.

The crystal producer uses real itemless Objects and requires three distinct clicks per color and character/run. Actual native acknowledgement, rendering and reachability remain unaccepted. The garden conversion preserves 32 unrelated collector rows, eleven unresolved ham visuals and suspicious pepper object 228415; underwater reachability remains qualified. Original geometry and variant fidelity are not proved by source checks.

[OBJECTIVE-COVERAGE.json](OBJECTIVE-COVERAGE.json) describes the 73 mercenary producer paths; [the prerequisite bindings](prerequisite9344/producer-bindings.json) describe the earlier 47. Source coverage does not mean the quests have passed native testing. Private checks, player fixtures, credentials and proprietary client assets are excluded.
