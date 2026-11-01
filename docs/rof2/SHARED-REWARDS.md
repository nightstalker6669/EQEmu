# Shared task rewards and ownership

This branch owns the provider-independent reward catalogue, selection session,
RoF2 display/claim/inspection serialization, task reward delivery and durable
occurrence ledger extracted from achievements commit
`1cfaaee4a2c02e2fb7c9992d971749790bfb83e7`. No achievement manager, progress engine,
criteria/provider code or achievement content is enabled here.

`common/reward_display.{h,cpp}` owns reward wire models and serialization in
`EQ::RewardSelection`. The historical `OP_AchievementReward` name is retained
for the native claim-capable reward manager (`0x6411`); task previews use
`OP_RewardSelection` (`0x6471`). This protocol name does not activate achievements.
Task reward catalogue, custody-aware delivery, lore preflight, persistence
results and wire reward-set identity retain the published behavior.

`METHODSELECT = 3` requires RoF2 and a validated reward set. Pending task rewards,
including common-only sets, are restored after character data loading. Shared
completion is archived before per-member rewards, with canonical occurrence
ownership surviving removal/rejoin. Task Collect uses physical inventory as a
high-water mark, and progression options freeze the distinct qualifying task
count with the durable reward occurrence. Rechecking a stack or repeating the
same qualifying task cannot multiply credit.

## Schema compatibility

Run the matching binary database updater through version 9334. Published
migration IDs 9331 and 9332 and their exact SQL bytes remain compatibility
envelopes. They historically mix generic reward DDL with empty achievement
schema scaffolding. Keeping those definitions intact prevents a core-only
install from stamping their IDs while leaving a subsequent achievement install
without required tables. Empty scaffolding is not achievement content.
Versions 9333/9334 retain the published task progression state/content changes.
There is one registration per migration, with the original names and ordering.

The original additive SQL files also retain their exact identities:

- `utils/sql/git/2026_10_27_achievement_reward_classes.sql` adds the generic
  `reward_options.class_mask` field; its historical name remains unchanged.
- `utils/sql/git/2026_10_30_shared_task_reward_identity.sql` supplies durable shared
  occurrence ownership in the player database.
- `utils/sql/git/2026_10_30_shared_task_admission_policies.sql` supplies optional
  task admission policy in the content database.

Those three existing supplemental files are not newly registered under divergent
IDs. Apply them through the established reviewed feature install sequence before
using the associated task capabilities. Existing deployments must review/adopt
the matching schema rather than create new ledgers or clear active leases.

Achievement consumers retain their provider callbacks in the contextual client
packet dispatcher and expose source-compatible aliases for the moved wire API.
They must link the shared codec once and inherit this exact core revision.
Consumer updates must merge shared client/header/packet changes contextually;
copying integration headers would import unrelated housing code.
