# Mesa interaction packets

These existing RoF2 mappings connect the quest scripts to native interactions. They are **not newly recovered opcodes**. Values come from the release's RoF2 patch configuration; layouts below are from `common/patches/rof2_structs.h`. Native gameplay exercised the routes; this package contains no packet captures.

| Opcode | RoF2 value | Use |
|---|---|---|
| `OP_ClientUpdate` | `0x7dfc` | Movement and exploration/proximity |
| `OP_ClickDoor` | `0x3a8f` | Food sacks, moving bush and moving rock |
| `OP_ClickObject` | `0x4aa1` | Existing brewing barrel and ground pickups |
| `OP_ItemPacket` | `0x368e` | Native supplied/collected items |
| `OP_ItemLinkResponse` | `0x70c0` | Native item details |
| `OP_TaskDescription` | `0x3714` | Accepted task journal |
| `OP_TaskActivity` | `0x08d3` | Objective progress |
| `OP_TaskSelectWindow` | `0x705b` | NPC task selection |

The [main protocol reference](../../protocol/README.md) supplies the achievement and reward mappings/structures used by task completion, parent progress and reward claims.

## World-object click: 8 bytes

```cpp
struct ClickObject_Struct {
    uint32_t drop_id;    // offset 0: runtime world-object ID, not database ID
    uint32_t player_id;  // offset 4
};
```

The server resolves the runtime object before `EVENT_CLICK_OBJECT`. Mesa then checks its original database ID, model, type and position. The brewing barrel is object **227920**, model **IT10805_ACTORDEF**, type **19**. `Object::FixZ` snaps it to terrain; its runtime height differs from the stored database height. The original crafting interaction remains active.

## Door click: 16 bytes

```cpp
struct ClickDoor_Struct {
    uint8_t  doorid;          // offset 0
    uint8_t  unknown001;      // offset 1
    uint8_t  unknown002;      // offset 2
    uint8_t  unknown003;      // offset 3
    uint8_t  picklockskill;   // offset 4
    uint8_t  unknown005[3];   // offset 5
    uint32_t item_id;         // offset 8
    uint16_t player_id;       // offset 12
    uint8_t  unknown014[2];   // offset 14
};
```

Unknown fields retain their original names; no new semantics are inferred. Mesa validates the original door database ID/model, actual reach, quest state and required equipped item where applicable.

## Proximity timing

`Client::Handle_OP_ClientUpdate` calls `EntityList::ProcessMove` before replacing the client's stored coordinates. An `EVENT_ENTER` handler can therefore see the preceding position. Resource entry queues a one-second NPC timer, then rechecks the actual client, resource identity, position, task and inventory. A character who leaves before that check receives no item or credit. This fixes event ordering without changing a packet layout.
