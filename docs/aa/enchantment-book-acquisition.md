# Enchantment book acquisition

Six current grant-only enchantment abilities have matching books that invoke spell 11370 (`Learn Ability`) and reference missing item-script handlers. Add six `global/items/script_<ScriptFileID>.lua` entrypoints and a bounded `lua_modules/aa_enchantment_books.lua` helper. The helper accepts only the six mapped books; each entrypoint also requires its own exact item ID.

| Book item | ScriptFileID | Ability | Minimum level |
| --- | --- | --- | --- |
| 64554, Enchant Alaran Metal | 20561 | 708 | 90 |
| 64555, Mass Enchant Alaran Metal | 20562 | 709 | 91 |
| 76276, Enchant Planar Alloy | 22568 | 1800 | 95 |
| 76277, Mass Enchant Planar Alloy | 22569 | 1801 | 96 |
| 81136, Enchant Feymetal | 16494 | 754 | 80 |
| 81137, Mass Enchant Feymetal | 16495 | 755 | 81 |

These associations compose the current exact item and ability names and their metadata. They are **selected local acquisition behavior, not recovered original scripts**. The minimum level is the maximum of the current item required level, click level and AA rank level. Alaran and Planar item requirements exceed their click and AA requirements by ten levels; the script explicitly preserves the higher requirement. No item or AA metadata changes.

The cast-stage handler requires Enchanter class 14 and current AA rank zero. It captures scalar slot, item ID and serial, verifies that the same live instance is stackable with positive charges, then calls `GrantAlternateAdvancementAbility(ability_id, 1, false)` once. The native API retains its eligibility and payment checks; all six current rank costs are zero. The handler confirms rank one and rechecks the original stack after synchronous purchase callbacks.

Only a confirmed grant consumes one unit from that verified stack with `DeleteItemInInventory(slot, 1, true)`. Quantity zero would remove the entire stack and is never used. The handler never accesses the old item wrapper after deletion. A fresh inventory lookup detects an empty slot and calls `InterruptSpell(11370)` to cover the modern packet path's early return. If a partial stack remains, the nonzero return reaches ordinary native cast-veto recovery without another script interrupt.

AA grant/save and inventory deletion/save are independent operations. A moved or changed stack after the grant is retained and reported for GM review; the script does not compensate, retry automatically, or provide a durable transaction across AA and inventory state. Existing player cast callbacks can overwrite the item veto; none was found in the checked `global/global_player.lua` and `.pl` sources. This implementation preserves that reviewed dispatch constraint.

Validation reuses the accepted AA006 and enchantment interface review. Final Lua 5.2 syntax passed for all seven files. Two focused offline scenarios confirmed one-unit deletion and the empty-slot versus partial-stack recovery distinction. No eligibility matrix was repeated. Native acquisition, consumption persistence and UI behavior remain unobserved. The candidate does not change core code, global player hooks, NPCs, spell definitions, item records, AA rows or player state.
