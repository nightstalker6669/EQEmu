# Poison and Alchemy audit checkpoint

This partial checkpoint records the audits through RRC068. It adds no SQL execution. The installed additions total 2,613 recipes and 14,106 entries across 62 new-recipe batches and three container-extension batches. Existing corrections cover 16 ingredient item IDs and eight failure/salvage pairs. The RRC068 readback contains 25,436 recipes and 191,725 entries.

The additional 50 basic Alchemy distillates already match, bringing the reviewed basic ranks I-X total to 120. Poison dye, Other, OoW, GoD and quest reviews distinguish represented formulas, the applied RRC064-068 changes, and unresolved quantities, minima, equipment or per-player mechanics. Existing restrictions remain in place where the maintained source omits them.

Charged Poison has 63 maintained source formulas: 40 existing definitions match ingredients, dose charges, returned vials, Coffin Poison Bottle and no-fail flags; 23 lack an existing output producer and remain held for review. One five- or ten-charge item is not five or ten items. The server passes recipe successcount to SummonItem as charges, so changing these existing counts to source Yield1 would create partly charged bottles. Existing numeric trivial and consumed-container settings are preserved.

The attached RRC068 receipt records four Marble Mortar links with all four existing recipe definitions, 20 old entries and learning flags preserved. There was one guarded SQL invocation and exact affected postimage readback. There were no native client or player-item actions.

The maintained catalogues and mutable historical guides support source review; they do not prove immutable May2013 values or native acceptance. Coverage remains incomplete.
