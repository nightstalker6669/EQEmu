# God Learner: existing RoF2 item and text messages

This is a source audit of the existing RoF2 codec, not new opcode discovery or proof that Live uses either text renderer for these shards. The actual lore-window renderer and grant-versus-read achievement trigger remain unresolved. No new opcode mappings were installed.

| Existing message | RoF2 value | Source role |
|---|---:|---|
| OP_ItemVerifyRequest | 0x189c | Client item-use request; server validates actual inventory item |
| OP_ItemVerifyReply | 0x097b | Existing server reply |
| OP_CastSpell | 0x1287 | Existing spell-cast route; a request is not completed-use proof |
| OP_ReadBook | 0x72df | Existing book/scroll text message |
| OP_OnLevelMessage | 0x575b | Existing titled popup message |

Values come from `utils/patches/patch_RoF2.conf` in the inspected source. Do not substitute these values into a current Live mapping.

## Concrete item-use callback

QuestParserCollection resolves nonzero ScriptFileID to `items/script_<ID>.lua`. For these nine shards the shared path is `items/script_20970.lua`. Lua item events expose `e.owner` as Client, `e.self` as the actual ItemInstance, and `e.slot_id` as the supplied inventory slot.

The eight achievement shards have click spell12483, type1, unlimited charges. Hidden Three66791 currently has script20970 but no click spell (effect-1/type0/maxcharges0). OP_ItemVerifyRequest reaches EVENT_ITEM_CLICK for supported inventory slots even when its spell ID is not positive. EVENT_ITEM_CLICK_CAST is before CastSpell; it does not prove a successful spell or displayed lore. Native client emission for script-only Three still needs verification.

The candidate only allows lore output when the native supplied slot contains the same item ID and serial. Reading never summons/deletes items, consumes charges, modifies quest buckets or awards achievement progress. The normal-use route does not depend on the Shard of Three having an invented spell effect.

## Popup: OP_OnLevelMessage

Native `OnLevelMessage_Struct` fields:

```cpp
char Title[128];
char Text[4096];
uint32 Buttons, Duration, PopupID, NegativeID, SoundControls;
char ButtonName0[25], ButtonName1[25];
```

The RoF2 wire structure adds uint32 count fields before Title, Text and each button name. It uses uint8 Buttons and SoundControls, followed by duration/IDs and remaining unknown fields. The existing encoder fixes count fields at128,4096,25,25 and copies the existing fixed buffers. Use that codec instead of serializing the native structure directly.

`Client::SendFullPopup` returns without sending when the title exceeds127 bytes or text exceeds4095 bytes. Its Lua Popup wrapper returns void. The candidate rejects oversized or embedded-NUL text/title before that call; a void return is not a client display/read acknowledgement.

## Book: OP_ReadBook

Native server `BookText_Struct` is variable length:

```cpp
uint8 window, type;
int16 invslot;
int32 target_id;
int8 can_cast, can_scribe;
char booktext[1]; // variable text
```

The inspected RoF2 encoder uses its larger `BookRequest_Struct` for the outgoing message, despite the existence of a short RoF2 `BookText_Struct` declaration:

```cpp
uint32 window;
TypelessInventorySlot_Struct invslot;
uint32 type, target_id;
uint8 can_cast, can_scribe;
char txtfile[8194];
```

It converts new-window0xFF to0xFFFFFFFF and inventory slots with ServerToRoF2TypelessSlot, then NUL-copies into txtfile. The candidate therefore limits book text to8193 bytes and rejects embedded NULs, preventing silent codec truncation. Known type values are0scroll,1book,2item-info; the actual shard renderer/type remains unbound.

QuestReadBook queues OP_ReadBook and returns void. Existing incoming read events and a queued output alone do not identify God Learner's completion trigger. No automatic OwnItem/LootItem criterion or credit on a pre-cast notification is supplied by this candidate.
