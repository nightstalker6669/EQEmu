# Research and limits

The initial audit compared the local RoF2 spell data and database with Allakhazam expansion/class lists and Raidloot class lists. Allakhazam/Lucy and historical guides were inspected through Chrome. Modern Live differences were treated as review candidates, not automatic corrections to historical spells.

- The database contained 40,722 spells, maximum ID 42,602. The RoF2 source limit is 45,000, so no spell IDs were remapped. The reported 50,000 concern did not require content changes.
- The initial expansion reference had 11,341 distinct entries. Its two missing entries were later 2014 additions and were excluded. This generic list was not exhaustive: the actual Greater mote quest lists documented Mark of the Exemplar II/III despite omission from the generic list, so both are included in the reward catalogue.
- 114 class-level differences against current sites were reviewed. Later Live changes and non-scribable helper/click records account for them. Spirit Salve gained Beastlord 48 in 2016; that later addition was not applied to the RoF baseline. This package changes no `spells_new` levels or effects.
- A large current-Live numeric effect comparison found differences requiring historical interpretation. They were not indiscriminately patched. Database dependency repair is not proof that every C++ spell effect has original Live behavior.
- Existing zone spell blocks and expansion boundaries were preserved. Later Ethernere teleport content was excluded.

## Acquisition and scroll items

[Rasper's RoF spell guide](https://www.raspersrealm.com/Everquest/RoF/miscSpells.html) supplies the class-vendor assignments, Magus Burlshin faction requirement, Kerkaz keywords and physical turn-in flow.

[Allakhazam quests 6004–6008](https://everquest.allakhazam.com/db/quest/6004) document the five Dreadmote tiers; [6009–6013](https://everquest.allakhazam.com/db/quest/6009) document Terrormotes. The lists include original 2012–2013 player reports. The package includes the RoF Shadow of Fear additions present in the local client. Source spelling mistakes were matched to the existing spells; the level-98 Shadow Knight bundle follows the source correction of Gift of Falsin plus Minion of Grelleth, not Falhotep's Covenant.

[Glorious Expurgation III](https://everquest.allakhazam.com/db/item/106830) and [Daring Escape III](https://everquest.allakhazam.com/db/item/109629) are documented Glowing Terrormote rewards with 2013/2012 collector dates. Missing item records were reconstructed from existing rank-II siblings with the correct rank-III effect, name and Lore/NoTrade properties; other template fields were inherited and were not all independently recovered. Their lore display labels match rank III.

[Almar's guide](https://almarsguides.com/eq/quests/rof/rkiiandrkiiispellrunes/) provides reward-window/return-mote evidence. Modern trade-down options were not imported because their original-era availability was not established.

The three Exigent summoned items use existing sibling templates and [Lucy item 64981](https://lucy.allakhazam.com/itemraw.html?id=64981), [64983](https://lucy.allakhazam.com/itemraw.html?id=64983), and [64984](https://lucy.allakhazam.com/itemraw.html?id=64984) evidence. These are item dependency repairs; the underlying spell records were preserved.

## Pet and aura reconstruction

[Measured magician pet statistics](https://www.eqprogression.com/magician-pet-stats/) provide selected TSS-era HP, maximum hit, level and focus anchors. [Archived Magician's Tower research](https://www.eqmagetower.com/forums/viewtopic.php?p=26442) was also reviewed. Complete verified RoF-era values were not found. The published model labels level/stat scaling, focus extrapolation and secondary values as estimates. It does not claim that these are original Live NPC records.

Aura parent-to-child links were checked against [Raidloot](https://www.raidloot.com/spells) and existing local effect spell IDs. Recipient/movement roles use existing native aura modes; range, timing, duration and visuals include estimates. Native ground-target placement and other C++ behavior remain limitations that this database-only repair does not solve. Existing 26 aura records were not replaced.

The pet payload creates no world spawns, merchants, faction assignments or loot. Its 3,084 NPC templates are referenced by the reconstructed pets/focus variants. This is functional reconstruction content, not a complete historical-balance recovery.
