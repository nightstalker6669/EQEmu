# Tradeskill coverage checkpoint: November 3, 2026

Five guarded content batches have added **110 recipes and 644 logical entries**, with independent full-field readback and preservation of all prior recipe/entry rows. This checkpoint also supplies RRC005 validation/application status. It performs no additional content or runtime changes.

The working scope is Original through May 31, 2013, including every tradeskill and relevant quest/container combines. The overall source universe is still unknown. The registry identifies 145 catalogue links and 13 general/cultural indexes; source links cover 124 of 260 skill/expansion cells. Link discovery is not recipe enumeration or historical approval.

The nine expansion-indexed RoF pages contain **1759 maintained source rows**. Full visible-text formula receipts have been captured for the eight ordinary skill pages, preserving ingredients, returns, restrictions and notes. The no-skill page has a grouped receipt pending mechanics review. Census counts are in census.json; they are not 1759 independent or historically confirmed recipes.

| Page | Observed rows | Current comparison |
| --- | ---: | --- |
| Baking | 96 | 83 installed; 13 remaining |
| Brewing | 16 | 10 installed; 6 remaining |
| Fletching | 44 | 8 installed; 36 missing formulas |
| Tinkering | 159 | 9 installed; 150 missing formulas |
| Pottery | 253 | 253 missing formulas; includes 45 unfired rows |
| Jewelcraft | 118 | 116 jewelcraft rows: 108 recipe gaps and 8 missing output items; 2 other rows belong to Research |
| Smithing | 404 | 404 missing formulas |
| Tailoring | 557 | 539 raw missing formulas, 11 missing output items, 7 disabled existing candidates; 551 distinct exact source formulas |
| No Skill | 112 | 16 ingredient/container signatures, each listed under 7 output patterns; mechanics unresolved |

Jewelcraft uses two separate tables. Pottery has three-cell fired and four-cell unfired rows. Tailoring repeats each of two shawl formulas four times, yielding six excess source rows already included in its status counts. Two disabled tailoring candidates match the listed formula fields; five use another ingredient/book variant. Existing identities and rows were preserved; those variants have not been dated or changed.

Nineteen verified native output IDs are absent from the item table, confirmed by fresh read-only queries. Their source links are retained in census.json. Recipe additions must wait for supported item definitions rather than invent stats or substitute similarly named items. The cultural no-skill source has contradictory displayed yields and a Dwarven bad-item return; charged-book and backpack semantics remain unresolved.

Maintained pages contain later edits, including a 2022 Live behavior note. Page placement, item import timestamps, and output identity do not prove May 2013 availability. Alchemy also had a 2006 overhaul: current Original/TLP lists require retired/alternate-ruleset review. Modern complete tradeskill-AA lists are not era-specific inventories.

The next work is era/gate/dependency review, general/class/cultural traversal, and all earlier expansion catalogues. Primary Planar Goo/Energy Shard acquisition remains a separate unresolved dependency. No native combine, complete historical coverage, or natural acquisition acceptance is claimed.

Sources: [EQTC recipe index](https://www.eqtraders.com/articles/article_page.php?article=g28), [RoF guide](https://www.eqtraders.com/articles/article_page.php?article=g567), and the individual catalogue/item links retained in the two JSON files.
