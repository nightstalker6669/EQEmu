# Protocol recovery and confidence

Target artifact: **eqgame.exe, May 10, 2013 RoF2**, SHA-256 `2a8702ad9f722704f01355c0750be7d6f164a8b9c9128ba0cf286ea32b405b0e`, preferred PE image base0x400000. Addresses below are virtual addresses for this exact build; they are not portable offsets for another client.

Recovery used REA/Ghidra with focused Capstone instruction decoding and exact-build source correspondence. Raw client binaries/disassembly databases and private investigation material are not distributed. The public interface descriptions and production codecs retain the recovered facts needed for integration.

| Claim | Native anchor / observation | Confidence and limit |
|---|---|---|
| Actions0x7121 and payload0x434 |Notify0x669440, callback0x668710, literal/send sites0x6695ce..0x669611 and0x668846..0x668872 |Native sender construction |
| MOTD+0x14 |Stack accounting0x669ba7..0x669bbb and bounded copy0x8dc4f0 |Corrected earlier+0x1c hypothesis |
| Invite uses spawn / roster actions use character ID |Invite target+0x148 versus roster+0x44c+i*0x54 |Different identity domains |
| Camp request0x0c12 |0x6688e0, literal0x668967 |Two u32 fields and exact8 bytes |
| Catalogue0x074d |Dispatch0x4d19e6 →parser0x6691d0 |Count and variable unaligned records |
| Roster0x6e4d |Dispatch0x4c55cd →predicate0x7f93b0 →copies0x4b3860/0x4b3890 |0x9e4+0x20 copied extent; no receive length equality recovered |
| Version5/Sync1 |Constructor0x7f9430 |Defaults, not database revision semantics |
| Leader first |UI0x667f48 compares first member ID to local unique character ID |Native control behavior |
| Invite0x62ab |Dispatch0x4c5733 →UI0x667db0; entity lookup0x5996e0 reads packet+0x0c |Field consumed; exact minimum receive size unknown |
| Vitality0x2cc6 |Dispatch0x4cd4d3 →0x4b7110 |u64 reads0/8, u32 reads16/24; high AA words unresolved |
| Gauge normalization |0x7633ca and0x763438 |Clamps then thousandths |
| Ten-level hint |Cap helper0x7bfce0, default read0x7bfe24 |Not complete Live cap formula |
| `/fsay` channel21 |Native sender0x5243e7 |Existing channel message path |

The roster field labels for name/handles/offline flags and camp type/flag use exact-build `SFellowship`/player source correspondence in addition to native copy offsets. The public layouts identify fields that are source-correspondence-only, unimplemented or unresolved. A label does not establish full server semantics.

REA evidence anchors retained in the local investigation:

- `ev_015ae63d678ebf20185e62c944997e5376b38cc52d37fc4b454428a0ea965bea` —action sender bytes.
- `ev_1e644a40f2e8ac2ff791379f2bd3d51699df68d94c08130f18dec3bc18915d61` —roster receive block.
- `ev_05704b14df7da1a0368048d4d45f15ddb4c76f3d6286146eab58f63a1517d9fd` —vitality consumer.
- `ev_ecb5b4333cd0b7c5e8dd63119ad3b01eec661586259ff6146f20ed5f448ca5de` —fellowship chat sender.

These hashes are provenance identifiers, not public downloadable assets. Independent reproduction requires the same lawful client build and analysis tools. Native three-client acceptance confirms implemented message use, not every unknown byte or Live policy. No nearby unmatched dispatcher case is reclassified as a missing fellowship feature without evidence.

Primary gameplay context: [2007 fellowship introduction](https://www.everquest.com/news/imported-eq-enus-50327) and [2011 fellowship XP description](https://www.everquest.com/news/imported-eq-enus-52340). These describe gameplay concepts; they do not prove byte layouts, current Live behavior, or this emulator's radius/conversion policies. Matching shipped campsite descriptions/categories and Standard spell formulas supplied numeric/template evidence; complete client data files are not distributed.
