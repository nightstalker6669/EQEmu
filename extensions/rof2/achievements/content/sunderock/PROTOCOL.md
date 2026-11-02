# Protocol and server-event scope

The existing [RoF2 achievement protocol reference](../../protocol/README.md) provides mappings and structures for catalogue definitions, state/progress/completion, comparison and reward selection. This content batch adds no new opcode number, packet field or client patch. Live opcode values are not copied into RoF2.

Task acceptance, native kill/loot/delivery credit, journal updates, completion persistence and reward choices use the branch’s existing task and achievement handlers. The caravan barrel uses the already-mapped RoF2 `OP_LDoNOpen` (`0x3d5c`), the existing chest handler and actual corpse loot. The note offer waits until inventory custody follows the pre-transfer loot event.

The egg uses the existing ground-object creation/pickup path. `EVENT_PLAYER_PICKUP` can veto the native transfer while leaving the ground item in place. Successful transfer and loot credit remain native. These Lua event contracts are server interfaces, not new wire structs.

`Mob:SetTimer` takes seconds; `Mob:SetTimerMS` takes milliseconds. The exploration and deferred-note player timer uses one second. Courier actor timers use the quest API’s millisecond interval. Timer units are covered by an offline contract check and native scouting progression.

Native acceptance is recorded separately from static/source and installer evidence. No raw captures, client binaries, QA scripts or player records belong in the public package.

The matching engine must include quest-created one-shot ground-object loot credit: `EntityList::CreateGroundObject(item_id, ...)` marks the server object, and successful pickup credits the existing native loot objective after all veto/custody/lore checks. Player-dropped objects are excluded. This flag is server-only; it adds no client packet field. The installed lifecycle module restores an uncollected egg after zone state resume and marks actual custody to avoid recreating a collected egg during the same original tender lifetime.
