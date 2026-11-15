# Oceangreen Journeyman II acquisition consumer

## Coordinated policy

This consumer is part of the accepted ten-task Oceangreen release. The existing `SodProgression:EnableOceangreenSlice` rule defaults off in source; the matching reviewed schema/content installation enables the slice. Native acceptance included earned progression and an ordinary Journeyman II healer hire. Install the complete matched code, quests and database inputs; see [accepted scope and upgrade instructions](../../docs/rof2/OCEANGREEN.md).

When the rule is off, offer/hire behavior is unchanged. When enabled, every normal liaison's new Journeyman II offers and hires require `Client::GetSodJourneymanTierState(2) == 1`. Identification uses the server template's `ProficiencyID == 2 && TierID == 2`, not client text, displayed grade, class or guessed template IDs. Apprentice, J1 and J3–V follow their existing policies. Higher tiers are not inferred from J2, nor retroactively restricted by this first slice. Those still-permissive other tiers are an explicit scoped policy boundary.

The only progression bypass is active server GM mode (`GetGM()`). Elevated account status without active GM mode still uses normal progression. The bypass skips only this new filter: all existing global/zone mercenary rules, client-version/template checks, shroud handling, raid/group/combat checks, merchant class/range/offered-template validation, contract capacity and cost checks continue to apply. Existing administrative command/ownership-transfer paths are untouched.

## Exact resolver contract owned by Spells

Required public API already supplied in r4:

```cpp
int Client::GetSodJourneymanTierState(uint32 tier);
```

For `tier == 2`, 1 means positively complete; 0 means incomplete; -1 means unavailable/unknown. Every value other than 1 denies a new ordinary J2 acquisition. The consumer makes one lazy read per offer-list evaluation if any J2 template exists, and a fresh evaluation at hire time. No history result is cached between shop and hire, between characters or across sessions. A database/read failure at hire therefore cannot reuse a previous successful shop result.

Spells owns committed native history/archive reads, the feature rule and this API. The bound Oceangreen GROUP tasks are 920177 Familiarity is Key, 920178 Stop the Contamination, 920179 A Dread Challenger and 920180 Beat the Blackburrow Boss. All four personal group-theme completions are needed. Sergeant requester tasks are not personal theme requirements. Raid completion, reward delivery/claim status, other themes, zone access, temporary in-memory task progress, hails and account privileges are not substituted for this result. No duplicate resolver or new entitlement writer is implemented here.

The published integration preserves the later character-selection fix, mercenary resource/level100 behavior and unrelated source. The resolver and consumer belong to the same matched server revision.

## Offer and hire paths

`Client::FilterSodMercenaryOffers(std::list<MercData>&)` filters an already client-version-eligible, server-derived liaison offer list. `Handle_OP_MercenaryDataRequest` applies it before counting/encoding rows, stances and merchant slots; empty grades are removed only when filtering changes the list. `CheckCanHireMerc` applies the same policy to a freshly obtained offer list before accepting the requested template or reaching payment. A direct or forged hire request cannot bypass the UI filter. A refused J2 request uses the existing not-offered response and does not charge or create a contract.

The filter does not add templates that a liaison did not previously offer, grant funds or bypass other restrictions. Owned roster display, spawn, unsuspend, zone following, upkeep, suspension, persistence and transfer remain unchanged. No existing owned J2 is revoked by a failed read, disabled slice or incomplete theme. The accepted resource-bar and level100 changes are retained in their exact preimages.

## Accepted scope

The matched local native cohort completed all ten tasks and verified shared rewards for two recipients, earned Void B access and ordinary Journeyman II acquisition at normal player privileges. Existing fresh-login persistence evidence was retained. See the linked release guide for the bounded acceptance and upper-ward physical-blocking limitation. Other tiers retain their existing scoped policy, and platform packaging success is separate from native gameplay acceptance.
