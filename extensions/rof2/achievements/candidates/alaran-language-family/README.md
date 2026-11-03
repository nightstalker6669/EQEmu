# Alaran language family — disabled source checkpoint

This candidate adds the four period-supported lower language milestones to the existing Master training proposal. It is not installed or enabled. Migration9343 is locally reserved but unregistered; Master9342 and the earlier published checkpoint are unchanged.

| Achievement | ID | Actual Alaran proficiency required |
|---|---:|---:|
| Aware | 1900045 | 25 |
| Novice | 1900046 | 50 |
| Fluent | 1900047 | 71 |
| Expert | 1900048 | 90 |

Each is worth10points. Individual [Aware](https://everquest.allakhazam.com/db/quest/5838), [Novice](https://everquest.allakhazam.com/db/quest/5839), [Fluent](https://everquest.allakhazam.com/db/quest/5840) and [Expert](https://everquest.allakhazam.com/db/quest/5841) pages were entered September3,2012; September5,2012 comments independently repeat their threshold and no-extra-reward statements. Mutable modern headers do not establish minimum character level or original callback behavior. The 2015 Words comments are postcutoff and are not used as period trigger proof.

## Implementation

One data-driven evaluator uses the prior language value from the existing Words of the Unspoken training branch and a fresh native language25 readback after its setter. It checks crossed thresholds rather than exact equality:70→75 crosses Fluent71. It reuses the existing Master helper at99→100 and adds no duplicate Master criterion. The SQL adds only four disabled manual Boolean criteria, with exact disabled-parent/10point/required-component/collision/reward-policy/InnoDB guards. Existing parent and component rows, rewards, characters and housing are not modified.

The existing setter is absolute, using value1 rather than incrementing a counter. This Boolean representation is an emulator implementation choice, not a recovered Live numeric counter. Direct language setters do not emit the chat-learning quest event. No new generic event, opcode, class-skill substitute, login/history credit, optional-component credit or already100 reconciliation is introduced.

The patch changes only the accepted training branch's helper call to pass its prior proficiency. Existing training rates, recipes, material custody, task updates and already100 translation behavior remain unchanged. Both helper flags remain false. Current Live confirmed Master during training-to100, before final translation/hand-in/primer read; that does not prove the award timing of all four siblings or the original2012 callback. Existing [RoF2 language opcode/structure notes](../alaran-master/protocol/README.md) remain applicable; no new mapping is claimed.

## Important remaining failure gate

The ordinary caller discards its prior value and ignores the aggregate result. If a threshold update fails or is deferred at70→75, the next75→80 training call does not automatically retry that71 crossing. Repeating a mock absolute setter proves no counter increment; it does not implement recovery. An accepted native setter return may mean deferred RAM work, not durable SQL. Actual persisted results, failure behavior and relog state must be verified before completion is claimed. No new recovery framework or historical backfill is supplied here.

## Verification and next integration

Focused pure Lua checks and three syntax loads passed, covering25/50/71/90,70→75, Master reuse, invalid prior/readback, no gain, repeated absolute values and setter refusal. The isolated9343 SQL run stopped in resource preflight before creating its temporary database; no SQL cases executed. SQL guards/rollback/procedure cleanup and native sibling progress/custody/relog acceptance remain pending. Test code, private fixtures, raw observations and player data are excluded from this checkpoint.

The next batch must verify guarded SQL insertion/refusal/rollback, then ordinary in-game training transitions, actual material conservation and persisted required-component/parent state followed by relog. Any initialized test state must be labelled a synthetic precondition, not proof of the whole historical quest. An observed recovery failure must be reported as such; do not silently grant historical credit.

## Review-only placement and dependencies

This package is outside production `content/` and installer manifests. The `.disabled.sql` suffix alone is not protection from a generic SQL glob. Nothing here installs quests, registers an updater entry, changes a database version or enables a criterion.

`review/words-language-family.patch` requires the exact external runtime preimage in `review/bundle.json`; a matching public target has not been established. Do not apply it to an absent/different file. Any separately approved installation must replace helper, data and caller coherently, never caller alone or mixed Master/family versions. Stage/hash-check under an assigned quiescent affected-zone window, initialize a fresh Lua VM to avoid stale package caches, and roll back all three to exact preimages together. There is no deployment authorization in this source checkpoint.
