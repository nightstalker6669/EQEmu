# Protocol and server events

The existing [RoF2 achievement reference](../../protocol/README.md) includes the recovered opcode mappings and packet structures for catalogue data, progress, completion, comparisons and reward selection. This Vergalid batch adds no wire opcode, packet field or client patch. Modern Live opcode values are not substituted for RoF2 values.

Journal offers, acceptance, actual kill/loot/delivery objectives, completion persistence and reward choices use the branch's existing task and achievement handlers. Gygun's original chest uses `OP_LDoNOpen` and native corpse loot, followed by an actual hand-in. The original elevator's zone-specific Perl callback is retained.

The shard prerequisite uses the existing completed item-cast Lua `EVENT_CAST` contract. The engine checks original item87222, spell9486 and zone404 after successful native instant-cast completion; the event carries the existing source-item, item-slot and charge-consumption fields. These are server event fields, not new packet structures. The content checks the crystal location and actual personal/banked permanent-key custody before individual travel within the same instance.

Scene movement uses real `NPC:MoveTo` observations. Restored lease-tagged actors reconstruct timers and unfinished movement through the global spawn callback. No missing actor is converted into a kill, loot, objective or reward. Player and actor `Mob:SetTimer` intervals are in seconds.

All seventeen routes completed through native gameplay, including the shared orb task and item rewards. Coverage and remaining platform/multiplayer limits are recorded in provenance/native-coverage.json. Offline checks alone do not establish gameplay, historical combat balance or spawn rates.

The original chest still uses native `/open`. The combat-only orb blocks that shortcut with existing OpenImmunity53: `Client::HandleLDoNOpen` checks it before the treasure-kill path. Actual combat and native kill credit remain authoritative; no packet format changes.

Unearthed Power uses the existing shared-task protocol and native task type1. The server Lua `Client:GetSharedTaskIdentity()` returns the full native64-bit identity as a decimal string; it is never converted to a Lua number. Scene state is keyed by that occurrence so members share the encounter and different parties cannot collide. These are server ownership rules, not new opcode or structure fields. Native shared-task kill, completion, archive and reward handlers remain authoritative.

The orb uses server class1 with DisableMelee39 and zero movement/aggro radii. Chest class62 and AggroImmunity24/25 prevented native damage-owner attribution during acceptance. This is a content-profile correction, not a new opcode or synthetic task-credit path.
