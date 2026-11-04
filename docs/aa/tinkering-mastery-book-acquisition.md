# Tinkering Mastery book acquisition

Books 98471, 98472 and 98473 already use ScriptFileID 3751 but the checked quest tree has no matching handler. Add `global/items/script_3751.lua` to acquire the first three ranks of ability 575 through their intended books. Preserve the ability's grant-only flag and existing item, spell and Rylin Coil quest data.

The cast-stage callback accepts only those three books, a base-race Gnome of at least level 59, the exact preceding rank and enough unspent AA points. It calls `GrantAlternateAdvancementAbility(575, target_rank, false)` once so native acquisition checks and payment own the 3/6/9 point costs. The explicit preceding-rank check prevents the native grant loop from buying several ranks from a later book.

Only a confirmed target rank consumes the clicked book. The handler verifies the original slot, item ID and serial before the paid grant and again after AA purchase callbacks. It removes that whole nonstackable instance with quantity zero; quantity one only reduces charges on an unlimited-charge book. Other copies remain untouched. It does not access the item wrapper after deletion.

The callback returns a nonzero cast veto for these books. A player `item_click_cast_client` callback can overwrite that result; none was found in the checked installed player scripts. Packet dispatch review found no further instance dereference after the successful consumption path without that player callback. The modern packet path returns as soon as the clicked slot is empty, before its usual spell-bar reset. After consumption, the script therefore calls `InterruptSpell(11370)` to reset casting state and send the native interrupt and spell-bar enable packets. Client UI recovery still needs native observation.

AA purchase and inventory deletion are separate operations. This script does not provide a durable transaction across them. If a purchase callback moves the book, the paid rank remains, no other item is removed, and the player is told to contact a GM. No automatic compensation or retry is attempted.

Validation: Lua 5.2 syntax and 11 focused mocked scenarios passed before the reviewed addition of the native interrupt call, covering paid ranks, acquisition guards, failed grants and callback-induced item movement. Tradeskills review accepted those checks and requested the interrupt call using the existing Lua binding. Final syntax passed; the scenario matrix was not repeated. These checks do not prove native acquisition or UI behavior. This is an isolated source candidate; it has not been installed or deployed.
