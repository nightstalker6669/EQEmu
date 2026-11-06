# Owned transient doors

Scripts can create an in-memory scene door and later remove only that exact owned creation:

```
handle = eq.create_transient_door(owner_token, model, x, y, z, heading, open_type, size)
-- On success: handle.entity_id is the server entity ID; handle.token is a fresh creation nonce.
-- On failure: nil, with no door-list mutation or client refresh.
removed = eq.remove_transient_door(owner_token, handle.entity_id, handle.token)
```

Owner tokens must be nonempty. Model names must contain4–31 bytes without embedded NUL; coordinates/heading must be finite; open_type0–255 and size1–65535. Values are explicit, with no scene pose defaults. Creation requires an available engine entity ID and an unused client door ID1–255. Client IDs used by live doors or any DB row for the current zone/version or version-1 are excluded, including content-hidden rows. DB query failure, exhausted IDs or the existing500-door spawn capacity returns nil. No DB row is inserted or updated.

Private owner/nonce fields are populated only by the transient factory; ordinary and DB-loaded constructors leave them empty. Removal uses the server entity ID and requires DBID0, a marked transient, exact owner and exact per-creation nonce. A bare DBID0 or matching model is insufficient. Wrong/stale handles return false without mutation. A recycled entity ID cannot satisfy an old nonce. The existing Lua CreateDatabaseEntry method refuses marked transients, preventing insertion or accidental update of a DB row sharing the client door ID. Existing unrestricted legacy create_door behavior is unchanged.

EntityList owns the new Doors instance through its existing AddDoor/RemoveDoor/zone-clear lifecycle. Token state disappears with that instance; no separate registry, persistence, automatic pose grant or schema is added. Lua returns plain handle data rather than a retained raw door pointer.

Successful creation/removal uses existing DespawnAllDoors and RespawnAllDoors to rebuild client door lists. It never calls RemoveAllDoors or LoadZoneDoors: unrelated server door instances, entity/client IDs, open state, timers and other live fields are retained. Client rendering is briefly rebuilt, so uninterrupted rendering is not promised. Existing packet encoders and live state_at_spawn handling are reused. RespawnAllDoors now deletes an empty/filtered failed spawn-packet allocation instead of queuing it.

Source checkpoint is based on shared-core262ca9c4f11e1fdbe66b68df862eaf1c7e503163. Exact public preimages and patch/whitespace checks are recorded; source review covers constructor defaults, owner and nonce checks, client-ID exclusion, persistence refusal, instance deletion ownership and empty-spawn handling. No new test matrix or build/runtime/Lua/native execution was performed. Build Chat owns compilation; deployment and native scene acceptance remain separate.

This API does not establish Xorbb ladder/elevated-note XYZ, heading, collision or climb route. No ladder is spawned, no guessed pose is enabled and no fake climb or location grant accompanies this source change.
