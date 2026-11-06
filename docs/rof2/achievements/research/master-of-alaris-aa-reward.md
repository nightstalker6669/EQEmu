# Master of Alaris: unfinished earned AA-rank reward

Retained achievement1900210, Master of Alaris, requires completed1900065 Champion of Alaris,1900090 Paragon of Alaris and1900205 Veilslayer. Its three criteria use AchievementComplete event11 with count1. These are definition bindings, not proof that every prerequisite quest is implemented or a character has completed them.

January11,2012 patch evidence supports one Hero's Barracks ability704 rank and an ability rank cap4. It does not prove total mercenary roster capacity or the native IDs of missing successor ranks. Do not infer rank IDs from contiguous string IDs.

The retained catalogue has no reward-source binding for1900210. Existing reward type2 gives unspent AA points, not ownership of a specific AA rank, and must not substitute for this reward. Achievement completion already queues server-authenticated rewards and the existing reward ledger is unique by character/achievement/reward. Uncertain deliveries remain non-replayable.

A finite AA-owner implementation must serialize actual ability ownership and expected next rank against the earned-once claim, other earned sources and native purchases. The generic GrantAlternateAdvancementAbility helper uses an absolute target rank count, not a delta, and is not atomic with the achievement reward ledger; its downstream purchase path does not propagate SaveAA failure. Merely calling it and then marking the ledger is insufficient.

AA rank authentication/persistence/cache and mercenary capacity semantics remain with their respective owners. Already-at-cap and imported legacy completion/entitlement handling need an explicit contract. No AA points, native ranks, reward IDs, database rows or active rewards were added by this research handoff.
