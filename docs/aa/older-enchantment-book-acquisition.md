# Older enchantment book acquisition

Extend the AA009 enchantment-book helper's bounded table with ten current book-to-ability mappings and add their ten item-script entrypoints. The six AA009 entries and all helper functions remain byte-for-byte unchanged. Tinkering's separate AA006 handler is untouched. This is a new source revision; it does not alter AA009's retained publication/deployment manifest.

| Book item | ScriptFileID | Ability | Minimum level |
| --- | --- | --- | --- |
| 37589, Enchant Palladium | 3484 | 3208 | 51 |
| 37598, Enchant Palladium Trio | 3485 | 3203 | 52 |
| 37590, Mass Enchant Palladium | 3486 | 3204 | 53 |
| 37596, Greater Mass Enchant Palladium | 3487 | 3205 | 54 |
| 37599, Enchant Dwerium | 3488 | 3206 | 60 |
| 37600, Mass Enchant Dwerium | 3489 | 3207 | 61 |
| 39749, Enchant Temporite | 9846 | 3209 | 65 |
| 39750, Mass Enchant Temporite | 9847 | 3210 | 66 |
| 45974, Enchant Cosgrite | 13381 | 3218 | 70 |
| 45975, Mass Enchant Cosgrite | 13382 | 3219 | 71 |

Each selected book is currently Enchanter-only, stackable, with one charge per unit and click spell 11370. Its required level, click level and native AA rank level agree. Current AA rank costs are zero. The existing helper requires Enchanter class 14 and current rank zero, calls the native `GrantAlternateAdvancementAbility(ability_id, 1, false)`, confirms rank one, and verifies exact clicked slot, item ID, serial and positive stack charges before grant and before consumption. It deletes one unit with quantity one. Only an empty slot gets an explicit `InterruptSpell(11370)`; a remaining partial stack reaches normal cast-veto recovery. These accepted AA009 semantics are reused without a new eligibility or stack test matrix.

Script 3485 accepts **only item 37598**. Anomalous item 37373 has the same name and ScriptFileID but different class and stack metadata. It is absent from the helper table and returns zero/unhandled before any grant or consumption. Its metadata and copies remain untouched.

These mappings are **selected compositions of current item and AA data, not recovered original acquisition scripts**. Retained original client strings corroborate all ten ability names and descriptions. Retained 2015 emulator catalogs also provide configuration provenance where their rows are present. Neither establishes original May 2013 acquisition or availability. Current expansion tags, including zero, are preserved and are not used as historical proof. No AA, spell, item, NPC or global player metadata changes.

AA grant/save and inventory deletion/save remain independent durable operations. The inherited moved-stack reporting and no-compensation/no-automatic-retry behavior remain intact. No durable transaction or native acquisition, persistence, consumption or UI acceptance is claimed.

Validation checks the table-only helper change, exact entrypoint mappings, syntax of the eleven Lua files, and the specific item 37373 exclusion. Existing AA006/AA009 interface and stack-recovery evidence is reused. No fresh Tradeskills review, broad test matrix or native action is required by this data-only extension.
