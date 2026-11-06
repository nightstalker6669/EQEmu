# Xorbb protocol dependencies

This quest batch adds no new wire opcode or client executable change. The configured RoF2 values are recorded in `configured-opcodes.json`; field layouts are in `rof2_xorbb_wire.h`. These document existing engine behavior and are not a new Live capture or proof that this batch works in game.

The owned itemless crystal path uses the normal object spawn and click messages. Its acknowledgement reuses the eight-byte `OP_ClickObject` payload with `drop_id = 0` and the clicking player's ID. It does not open the container UI or send `OP_ClickObjectAction` as a substitute. The latter's separate 92-byte RoF2 layout is included to make that distinction explicit.

`OP_GroundSpawn` uses a variable-length model name. Its known suffix normalizes server size 100 and size 0 to the same wire scale 1.0 in the retained encoder. That removes a source-level scale mismatch concern, but does not establish rendering, actor orientation or click reachability. The in-memory server `Object_Struct` and its old offset comments are not the variable wire layout.

Source references: `common/patches/rof2.cpp` (`ENCODE(OP_GroundSpawn)` and `ENCODE(OP_ClickObjectAction)`), `common/patches/rof2_structs.h`, `common/eq_packet_structs.h`, and shared-core commit `966b510f4467a7af83913e5bbaecc542a723654b` for itemless acknowledgement and click lifetime handling. The source header is a documentation artifact; native protocol acceptance remains separate.
