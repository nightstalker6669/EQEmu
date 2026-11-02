# Steppes protocol and quest-event scope

The branch includes the complete existing [RoF2 achievement opcode/structure reference](../../protocol/README.md), its mapping fragment and fixed-prefix header. This package adds no opcode values or client packet fields.

Achievement definitions, state/progress/completion and parent reward selection use the existing achievement/reward protocol. Shared missions use the existing task accept, activity update and reward selection handlers. Real ground pickups use GroundSpawn/ClickObject; native trap placement uses the existing item-cast path. No Live opcode number was transplanted to RoF2.

The server-only Lua EVENT_CAST extension appends `cast_item_id`, `cast_item_slot` and `cast_item_charge_consumed` after the original spell/caster/level/target fields. It reports completed native item-click custody and preserves the original fields. It is a quest event interface, not a new wire struct. The narrow instant scene spells are7230 and9483–9485; the adjacent portal-repair feature remains disabled.

Shared reward occurrence IDs are durable database identities tied to full-width world runs, not timestamps or packet IDs. Admission/archive transactions protect membership/completion state before notifications or active-row removal. These are server persistence changes.

Native solo-owner shared runs, handins, all five Yavia waves, all seven Conquering components and one parent two-item claim passed on the corresponding combined Linux engine. Native multiplayer/rejoin/fault-retry and uninterrupted recurring Yavia pickups remain explicit gaps.
