# HoT Paladin level 90 native scroll definitions

This repair inserts two absent native items: 125060, Spell: Earnest Fury Rk. III → spell 25371, and 125066, Spell: Force of Marr Rk. III → spell 25377. Both are Paladin level 90, internal rank 10. It changes no spell, merchant, task, reward, player, inventory, or lease rows. The original reward grouping remains unresolved; task 920129 keeps its existing five choices.

The reviewed complete Lucy raw tables map 138/137 of the 287 columns. The remaining 149/150 fields retain ordinary sibling defaults, explicitly labelled estimates in provenance.json. Blank native fields are unknown, not evidence for zero. Both histories begin October 13, 2010. Force of Marr's augslot4unk2 and augslot5unk2 use the native value 1. No new item IDs or rank II pairing workaround is introduced.

Run `python3 insert_scrolls.py` for an offline plan. Applying requires PyMySQL, a private EQEmu configuration, and a fresh private receipt directory:

```sh
python3 insert_scrolls.py --apply --config /private/eqemu_config.json --receipt-dir /private/paladin-scroll-insert
```

The helper requires peq_housing_test, transactional items storage, the reviewed 287 columns, unchanged full sibling templates, the correct native spells/Paladin levels, and absent target rows. It locks the definitions, writes a durable prepared backup, inserts both rows in one transaction, and checks their full readback before commit. Existing target rows always stop execution. Default execution never connects to the database. This remains outside manifest.stages; the general installer does not enforce these checks.

These native IDs are absent definitions, so insertion resolves their original identities without replacing another item or altering stored copies. There is no blanket serialized-reference absence requirement. This supersedes the earlier proposal's generic Housing clearance and separate default-acceptance holds.

A prepared receipt with no completed receipt requires inspection of the actual two rows before further action; never retry blindly. Retain both receipts. Do not automatically delete the new definitions after they can be acquired: rollback would require confirming exact inserted rows and absence of new item copies/references. Item-cache refresh is a separately coordinated runtime action following successful commit. Database insertion and cache availability do not establish native exchange acceptance or resolve historical reward grouping.

## Applied status, November 4, 2026

Both native definitions are committed in peq_housing_test. Exact full-row readback and an independent spell/Paladin-level join passed. Build confirmed both definitions in actual world and all six zone item maps after an item-only refresh. No spell-cache rebuild, task/reward change, player/inventory/lease write or native exchange claim is included. The existing five Paladin reward choices remain unchanged pending original grouping evidence.

The first transaction rolled back before commit because Python datetime objects were compared directly with JSON strings. Target absence was confirmed; the corrected helper compares the established canonical row digests, and its second transaction committed both rows. Prepared/completed receipts and backup templates remain private. The corrected helper is published; do not use the earlier direct-equality revision.
