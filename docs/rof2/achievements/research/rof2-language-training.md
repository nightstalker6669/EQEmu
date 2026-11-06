# RoF2 language-training eligibility

Offline analysis of supplied client SHA2562a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e found a race/level eligibility predicate in the native training list. Wizard12 uses trainer31, because trainer class equals player class+19; trainer32 is for Magicians.

The language-list loop0x782530 calls predicate0x581800 before accepting a row. Its race table0xaea244 gives Goblin14 a threshold20 for Human race1. The indirect character method at+0x68 is interpreted as level from this table and class-specific8/20 gates. Native transition to an eligible-level Goblin row has not been accepted by this source finding alone.

The response call at0x4b4b0c passes payload+413 as language offer bytes. Goblin14 is payload427. The448-byte GMTrainee response overwrites its40-byte tail at408; literal byte19 is0xc9, which passes the client's unsigned>0 offer check. RoF2 inherits pass-through for OP_GMTraining(0x1966). No second offer-byte gap was found in this retained source boundary; an emitted runtime packet was not captured here.

When eligible, the normal route is the matching trainer's actual Goblin row and native Train button. The server language-training handler accepts a first point for zero coin and consumes one practice point. This finding authorizes no raw packet, skill or level grant, client patch or NPC class change. Original unknown-tail struct fields remain unchanged.

The successful evidence was bounded manual offline PE mapping/GDB disassembly after the REA provider's startup timeout. It is not a REA Evidence claim or native training acceptance. Private assembly and receipts remain outside this public package.
