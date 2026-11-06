# Lua pickup object identity

Both player and zone pickup callbacks now expose numeric `e.object_id`, taken directly from the existing `extra_data` argument supplied by `Object::HandleClick` as `GetID()`. It identifies the native scene object, not the item definition or a database row. Scripts can use the existing `GetObjectByID`/object entity-variable API to inspect scene ownership and stage.

The player callback retains `e.item`; the zone callback retains `e.item` and `e.other`. Dispatch, event return handling, pickup timing and inventory transfer are unchanged. Both callbacks run before transfer; the callbacks share the existing EVENT_PLAYER_PICKUP dispatch (the zone adapter is named handle_zone_pickup).

Source validation: two field additions only; removing them reconstructs the pinned public file byte-for-byte. Public Object::HandleClick supplies GetID() to both callbacks before inventory transfer. Patch applicability/whitespace checks passed. No build, Lua/native execution, opcode/packet/schema change or deployment accompanies this checkpoint. Native003 continues independently; compilation can be batched with pending core work or queued for the next coherent build.
