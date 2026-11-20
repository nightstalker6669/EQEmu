# Darkhollow Normal progression

This release includes 17 Normal quests, the Hive-earned Dreadspire entry device, level 68–70 Brother Hidden Path class spell or discipline rewards, Brovil and Jarzarrad progress for the Skull of Den Lord Rakban, and the Fall of the Illsalin shroud mission.

## Fall client append files

Use the RoF2 client files from your own installation. The server runtime ZIP includes these portable fragments under `database/client-data/rof2/`. The matched database ZIP carries the same fragments under `client-data/rof2/`.

| Append fragment | Client file |
| --- | --- |
| `spells_us.append.txt` | `spells_us.txt` |
| `dbstr_us.append.txt` | `dbstr_us.txt` |

1. Close the client and back up both original files.
2. Confirm the three spell record IDs and 15 display-string keys in the fragments are vacant. If the exact rows are already installed, keep them. Resolve any conflicting record before proceeding.
3. Append the corresponding new records to each file once. Keep the existing file's encoding and line endings, with a line break between its previous last record and the first appended record.
4. Reopen the client with the matching server, quest files, and RoF2 opcode map.

## Fall tuning and experience

Fall shroud stats, loadouts, and XP rates use reconstructed values. The tested shard-bearing payout was 5% of a level at full normal XP allocation or 90% of one AA point at full AA allocation, subject to caps. Directed RoF2 development checks covered shroud and original-profile transitions, Commander shard collection, XP settlement and repeat behavior. The current values are documented local tuning.

## Database upgrade

Use the matched database package and its `migrations/darkhollow-normal68-70/UPGRADE-CONTRACT.json`. The full seed is for an empty database. For an existing compatible Rathe r2 volume, use the ordered additive route through the level 69 baseline, preserve player progress and claims, and rebuild the item and spell caches against the matched database before startup.
