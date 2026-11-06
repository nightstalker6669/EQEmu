# Existing item-use protocol

| Message | Current RoF2 mapping | Payload |
|---|---|---|
| OP_ItemVerifyRequest | 0x189c | 12-byte inventory slot, then uint32 target; 16 bytes total |
| OP_ItemVerifyReply | 0x097b | 12-byte inventory slot, uint32 spell, uint32 target; 20 bytes total |

The slot contains six little-endian signed16 fields: inventory type, unknown02, slot, subindex, augment index and unknown01. See `item_verify_rof2.h` and EQEmu’s `common/patches/rof2_structs.h`; the RoF2 decoder converts the slot to the canonical server inventory slot.

Static REA/Ghidra analysis of the supplied RoF2 executable at0x697250 (identified by the matching SDK inventory-right-button offset) shows an ordinary-item branch sending message0x189c with16 payload bytes plus the2-byte opcode. No positive click-spell check appears on that send path. This is static control-flow evidence, not a captured ladder click.

The server’s `Handle_OP_ItemVerifyRequest` dispatches EVENT_ITEM_CLICK and EVENT_ITEM_CLICK_CLIENT before its no-effect handling. The client callback supplies the actual item, item ID and slot. Validate that concrete inventory instance before acting. Ladder66508 currently has clicktype0/effect-1; there is no evidence here requiring a dummy-spell or item-metadata change. Totem66518 and lens66521 already use23998, named Use Ability.

An item-use request is intent. It is not successful casting, placement, pickup or quest completion. Create the actual ladder/ground object first, retain real ownership and stage guards, and use native successful Loot handling for notes. All native acceptance remains pending. No opcode mapping is changed by this package.

## Existing door record and tilt

`OP_SpawnDoor` is mapped to **0x7291** and `OP_MoveDoor` to **0x08e8** in the current RoF2 patch configuration. `door_record_rof2.h` documents the existing **100-byte door record**, including model32, XYZ, heading, raw incline at offset48, size at52 and native door ID at60. Unknown bytes remain uninterpreted. No mapping is changed or newly recovered here.

REA/Ghidra on the supplied executable (`2a8702ad…b405b0e`) shows constructor `0x511990` reading record+48 as a signed integer, converting it to float and storing the switch tilt at +0x38/+0x54. Actor initialization `0x510960` forwards heading and tilt to actor creation `0x48b710`; model scale uses size/100. These are static observations, not native rendering/climbing acceptance or an angle-unit conversion.

The configured quest tree has raw incline125 and the pickup ladder130. Earlier upright model-only diagnostics omitted this field and cannot establish their world pose. The authored standing-ladder code now requires an explicit incline, verifies the existing tree tilt, and applies/readbacks tilt only on the exact freshly created owned native door. Never copy the pickup ladder tilt into the standing ladder without establishing its intended pose. No client assets or private analysis bundles are included. The protocol headers have not been compiled as a separate artifact on this host.

The same header includes existing `OP_ClickDoor` **0x3a8f** (16 bytes) and `OP_MoveDoor` **0x08e8** (2 bytes). ClickDoor carries native door ID, lock-pick skill, item ID at offset8 and player ID/reserved bytes at12. Static function `0x511d70` sends message0x3a8f with18 bytes including the2-byte opcode; this corroborates the existing16-byte payload. MoveDoor retains raw door ID/action bytes from the server codec; no new action semantics or capture is claimed. Native client IDs differ from SQL door row IDs and server entity IDs.
