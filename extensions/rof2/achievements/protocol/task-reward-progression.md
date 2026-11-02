# Progression-dependent task rewards

This engine extension supports quest families whose selectable rewards expand after different quests are completed. It reuses the existing RoF2 task reward packets and structures; it introduces no opcode mapping or packet layout.

The historical Buried Sea example is documented in the [March 2013 capture of the skeletal-hand guide](https://web.archive.org/web/20130316011050id_/http://everquest.allakhazam.com/db/quest.html?quest=4188). Six island families initially give experience/faction, then unlock one, two and three bone choices on their third, fourth and fifth different completed quests. Jardel's Hook is an exception with bones available from the first quest. The engine supports either policy through content rows; it does not hard-code island names or assume a fixed bone belongs to a particular quest.

## Content definitions

`task_reward_progression_members` lists the qualifying task IDs for each reward-bearing source task. The source must include itself. Only enabled non-shared task definitions are admitted. Repeated completions of a member count once.

`task_reward_option_gates` associates a canonical reward option ID with the minimum number of distinct completions required. Gates must refer to existing selectable options, with a positive threshold no greater than the number of members. Common rewards remain common. Options without a gate retain their ordinary eligibility rules.

Disabled task content can be staged without activating its progression policy. Invalid membership or gates prevent replacing the previous reward catalogue. Existing tasks with no progression content continue to use their static reward sets.

## Durable pending rewards

The new nullable `character_task_reward_selections.progression_count` is written in the same INSERT as the owned reward occurrence. The current completing task is included once. A repeated attempt to create the same occurrence preserves the original count.

When filtering leaves only common rewards, the task grants them automatically through the same owned delivery ledger; RoF2 does not show a task-choice page without selectable rows. Pending common-only delivery is deferred until the character profile has fully loaded and can be saved. An early login load must not change XP, money or item custody.

Reopening, reconnecting and claiming filter the reward options using this saved count. Completing other quests does not expand an older pending reward. A pending progression reward without its count, or a saved count whose policy has disappeared, is refused rather than silently given the entire reward set. Existing static rewards retain NULL.

Freezing eligibility at completion is an explicit server consistency policy. The historical sources establish the distinct-quest unlock rules; they do not establish every detail of the original server's deferred-claim timing. Do not present the latter as recovered protocol evidence.

RoF2 caches the displayed choices by reward-set ID. Each open reward occurrence therefore receives a separate client-facing set ID, retained when that session is refreshed. This prevents a later occurrence with more unlocked options from overwriting an older pending page. These IDs live only in the client session; reconnecting rebuilds them from the durable pending records. Claim and item-inspection requests resolve through that session identity before using the canonical catalogue and occurrence keys. No packet field or layout changes.

Reward option IDs remain canonical server identities. The existing wire-option resolution, occurrence ownership, class filtering and per-entry delivery ledger continue to authenticate claims. A new claim for an already-owned lore item is rejected before locking an option or delivering its common rewards. Retries of a locked delivery continue through their existing ledger, since an item may already have been delivered before another entry failed.

## Schema and rollout

Migration 9333 adds the nullable state column. Migration 9334 creates the two content tables. The normal updater routes state and content migrations to their respective databases. Apply the schema upgrade before starting the updated binaries. No existing completion, selection or delivery row is deleted.

Validation covered full Linux server compilation, the follow-up zone rebuild, isolated MariaDB migration/reapply checks, and native RoF2 checks using disposable content. Native checks covered bank exclusion, carried-quantity high-water collection, repeated-scan deduplication, common-only automatic delivery, different pending choice sets surviving reconnect, lore rejection before selection or common XP, the alternate valid choice, repeated-member distinct counting, and a static reward with no progression policy. The temporary content and account privileges were removed, and the test character’s original inventory matched its preimage. Native Windows execution remains unverified. Buried Sea quest/reward content is still staged separately; this engine change alone does not implement the 39 quests.
