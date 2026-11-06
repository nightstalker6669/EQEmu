# Bot AA duration: use the native focus result once

Status: source proposal only. This corrects compatibility between the inspected current source and native AA metadata; it is not a claim of historical player-client parity. Publication does not integrate, build, deploy or activate the patch.

## Problem and resulting behavior

`Bot::GetActSpellDuration` obtains percentage and tick duration focus through `GetFocusEffect`, then adds legacy hardcoded Reinforcement and Mesmerization bonuses. The generic focus calculation already includes the selected owned AA rank and its limits. The legacy additions count these AAs twice and bypass their filters.

`Mob::GetAA` resolves a rank ID to its owning ability and returns total ranks owned. Constants 86 (Reinforcement) and 266 (its former Mastery stage) both resolve ability21. Thus the Mastery boolean adds20 percent even when only rank1 is owned. Current native ranks86/87/88/266 provide5/15/30/50 percent via SPA128. For a beneficial100tick spell passing every AA filter, with no other focus, source arithmetic currently produces130/150/180/170ticks for owned ranks1–4; the focus-only result is105/115/130/150. These are analytical examples, not runtime measurements.

Mesmerization rank781 supplies SPA287 +1tick with mez-effect31 and minimum-duration5 filters. When that rank applies, the old override adds its owned rank count again; it can also add ticks when the focus filters reject the spell. Later ranks must use their actual selected-rank effect values, not a count-derived assumption.

## Bounded change

Remove only the two legacy direct-AA addition blocks in `Bot::GetActSpellDuration`. Keep its existing percentage/tick focus calls and integer return calculation. The generic evaluator continues to own effect magnitudes, limits and stacking. No changes to the player duration method, other bot behavior, definitions, ownership, points or charges. Existing bot rounding and discipline behavior are retained; this proposal makes no broader parity claim.

## Evidence and integration boundary

Inspected source head: `aedc7e878f325a8beaffb04c932fb3c1469f69d3`, branch `runtime/rof2-develop-20261030`. Relevant source: `zone/aa.cpp` (`Mob::GetAA`), `zone/bot.cpp` (`LoadAAs`, `CalcBonuses`, `GetActSpellDuration`), `zone/bonuses.cpp` (`ApplyAABonuses`), and `zone/spell_effects.cpp` (`GetFocusEffect`, `CalcAAFocus`). Fresh read-only AA metadata confirmed the rows above. Full private receipts and preimages remain with the AA owner.

Exact `zone/bot.cpp` preimage SHA256: `20d7e6c875278c5fe67275e3ff657c55dc200b4ae3bc75bf6c57c1a7ccc20354`.
Expected postimage SHA256: `0d68ba89eb46294fc56ea62a99f4f7f08e74f21a53d6aff9faef0b1e5b141b29`.

The forward patch passed `git apply --check` against that preimage. It has not been applied, compiled or accepted in a running server/client. No additional test suite was run. An integration owner must preserve unrelated source changes and reconcile drift instead of replacing a whole file. GitHub publication, Build integration and runtime acceptance remain separately routed by the coordinator.

The companion rollback reverses only this same removal. Use it against the corresponding postimage; preserve subsequent unrelated edits. No database rollback or player-data operation is involved.
