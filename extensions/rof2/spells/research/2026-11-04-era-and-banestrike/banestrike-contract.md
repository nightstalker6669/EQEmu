# Banestrike original contract research

Source-only; no spell/pet/NPC/AA/player mutation or native action. Original through-RoF architecture is activated AA15073 -> spell37668 -> temporary actor SumBaneSpirit. Exact original pet definition remains absent; a generic NPC would not implement the recovered contract.

## New period damage constraints

Chrome read 2026-11-04, official forums, contemporary first-person reports rather than developer source:

- https://forums.everquest.com/index.php?threads/how-to-upgrade-banestrike.2625/post-33953 (Vouivre, Dec26,2012): reports4500 damage at92 and94. This supplies two observed points, not the full bracket formula or all target eligibility rules.
- https://forums.everquest.com/index.php?threads/how-to-upgrade-banestrike.2625/post-34007 (Riou/EQResource, Dec26,2012): explains automatic level-dependent damage and8000 at100. The quoted6500 from another poster lacks caster level; no95-99 bracket inferred.
- https://forums.everquest.com/index.php?threads/how-to-upgrade-banestrike.2625/post-33714 (Tirratye, Dec25,2012): reports failure on raid enemies, including many deity avatars. Exact raid classification/exception predicate remains unrecovered; do not replace with an invented boss-name list or blanket rule without review.
- https://forums.everquest.com/index.php?threads/how-to-upgrade-banestrike.2625/post-33898 (Tirratye, Dec26,2012): describes summoned actor script using completed Slayer achievements to determine target eligibility. Community report corroborates pet architecture; not original script recovery.
- https://forums.everquest.com/index.php?threads/macros-and-banestrike.2374/post-30996 (Fanra, Dec15,2012): actually uses /alt activate15073 alongside a discipline, explicitly active rather than passive.
- https://forums.everquest.com/index.php?threads/how-to-upgrade-banestrike.2625/post-33575 (Dec24,2012): whole Skill/Special/Conquest sections each reduce reuse5sec. AA/Achievements owners retain grant/rank/criteria ownership.

A single fixed6500 or8000 pet hit, melee profile, or current Live auto-proc would not preserve these observations. NPC definition alone is insufficient: target Slayer qualification, level-dependent damage, raid exclusions, ability grant, and reuse progression need an explicit shared contract. No exact stats/level brackets/raid predicate invented here.

## Historical spell identity guard

Primary Lucy history https://lucy.allakhazam.com/spellhistory.html?id=37668&source=Live was read in Chrome earlier this continuation. Initial Live entry2012-11-23; December2023 reuse renamed Banestrike to Honorable Duel and removed SumBaneSpirit temporary-pet effect. Current37668 must not be imported over original Banestrike. History records July2014 range70->160; retained client range160 is not an authenticated May2013 baseline. Candidate original70 needs coordinated source review, not a silent repair. Original pet effect152/base1=1/max10/keySumBaneSpirit matches the original architecture; no exact pet row was recovered.

## Implementation boundary and unresolved dependencies

Retained TemporaryPets source requires a pets mapping and an NPC template. This is not an implementation of original Slayer qualification or level-dependent damage. Source inspection does not prove runtime behavior. The AA owner verified that the exact key is absent. Database owner completed a negative original2025-09-28 baseline archive lookup; do not repeat or infer a donor from an unrelated2026 movement-fix exclusion note. Exact actor identity/model, complete damage brackets, original raid predicate and first unlock/claim flow remain unresolved. No generic pet, new native identity or fixed-damage substitute is proposed. NPC/AA/Achievements owners retain their respective definitions/grants/criteria interfaces through the coordinator.
