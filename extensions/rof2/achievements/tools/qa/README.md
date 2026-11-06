# Existing local QA mallet access

This utility makes the **existing** item147495, JARVIS Achievement Test Mallet, usable by all16classes and16races (`65535` masks). It changes only `classes` and `races`. All combat stats, status restrictions, identity and existing player copies remain unchanged. It is a deliberately overpowered testing utility and has no historical Live claim.

This item was previously created locally; this utility does not create it and must not be run as part of the production achievement-content installation. If you do not already have the exact named item with its original access masks, the SQL stops without replacing anything.

Back up the affected item row, run `001-existing-mallet-all-access.sql` against your intended server database, and have your runtime owner refresh the item cache through the server's supported maintenance procedure. A database change alone does not update a running process or existing cached item instance. The installer does not stop clients or services.

Use `002-rollback-mallet-access.sql` to restore the original access masks, followed by the corresponding cache refresh. It refuses an unexpected item state.

When using the weapon for a quest check, preserve the character's original equipment and instance identities. Summon a temporary owned QA copy, use real attacks so native kill credit fires, then remove the temporary QA copy and restore original equipment. Do not use it to bypass a required encounter mechanic.
