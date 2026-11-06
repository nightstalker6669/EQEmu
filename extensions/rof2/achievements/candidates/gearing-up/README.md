# Gearing Up — disabled review candidate

Eight quests (parts I–VIII), 15 delivery objectives and eight required criteria for achievement500120600. Task IDs3044001–3044008; reserved migration9340. This package is unfinished and must not be enabled or copied into a running server yet.

## Included

`quests/` contains the family selector, one-active-task policy, physical hand-in candidate and exact replacement Tabazin adapter. `sql/` contains the guarded canonical migration, which inserts disabled tasks and criteria. `review/updater-entry.proposed.txt` is a registration proposal for a reviewed9339 descendant, not a patch to the current updater/version. `quest-manifest.json` and `payload-manifest.json` identify exact bytes.

XP1000 is the approved PEQ-style default, not a recovered original amount. Minimum level0 and faction0 are disabled schema placeholders; they do not authorize unrestricted admission or prove a historically zero faction reward. The admission policy currently denies access.

## Required before activation

- Resolve the native hand-in custody/coin gate. Native task delivery credit happens before EVENT_TRADE; a later exact-coin hand-in mismatch can return already-credited items. The ordinary failed-CheckHandin fix alone does not solve this. No proposed task-consumption API is supplied or assumed available here.
- Confirm eligibility and individual faction rewards. Sources conflict; do not enable a blanket modern value.
- Review remaining item acquisition provenance and current collision/preimage checks.
- Verify all eight quests, partial/excess/unrelated items, unwanted coin, completion removal, sibling admission and Screw Loose coexistence in game, followed by persisted-state/relog checks.

The Tabazin adapter depends on `achievement_quest_oculus` for existing Screw Loose routing. That dependency is not included or enabled here. Do not activate the unfinished Oculus package to satisfy it. The adapter replaces the exact reviewed predecessor; never concatenate it with another consuming trade handler. See `review/preimages.json`. The current active adapter has not been accepted as matching that predecessor.

## Evidence and provenance

Six actual isolated MariaDB/MySQL C API protocol cases passed: fresh disabled8/15/8 insertion; reapplication; collision; missing required item; nontransactional table; injected late insertion failure. Failure cases preserved rows and closed transactions. This is SQL-protocol evidence, not execution of the C++ updater or native conservation proof. Private fixtures, test code and player captures are excluded.

[Part II and historical family reports](https://everquest.allakhazam.com/db/quest/4399), [Gearing Up achievement](https://everquest.allakhazam.com/db/quest/5628), [Part IV](https://everquest.allakhazam.com/db/quest/5782), and [Rasper Fortress guide](https://www.raspersrealm.com/Everquest/SoF/openFortress.html). Current headers/guides are mutable. Current PartII+13, PartIV+7 and Rasper family+10 conflict.

Steel wire36485 is supported by [Lucy history](https://lucy.allakhazam.com/itemhistory.html?id=36485) and a March2008 report. Oil36486 is best supported by [item history](https://lucy.allakhazam.com/itemhistory.html?id=36486) and [a May2014 acceptance correction](https://everquest.allakhazam.com/db/item/71027); that correction is after the cutoff. Names alone do not establish accepted item identity. No loot or NPC placement changes are included.

## Repository placement

This is a candidate-only checkpoint under `extensions/rof2/achievements/candidates/`, outside production `content/` and its installer/manifest discovery. SQL files have `.disabled.sql` names for clarity; that suffix alone is not a safety barrier against an installer glob. No installer includes this directory, no root updater entry/version is changed, and no quest is deployed by this package. The migration IDs are reserved but unregistered. The review entries are documentary proposals, not active registration. Do not feed these files to an installer or copy Lua into runtime until the remaining gates and an explicit integration review are complete.
