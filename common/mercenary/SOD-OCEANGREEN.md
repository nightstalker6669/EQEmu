# Oceangreen Journeyman II acquisition consumer

## Coordinated policy

This narrow consumer is opt-in through the existing `SodProgression:EnableOceangreenSlice` rule, whose default is false in the Spells r4 source. No new rule, account flag, task history writer, reward or database migration is introduced. Activation belongs to the combined Oceangreen deployment, after local build/runtime acceptance and before GitHub publication as instructed by the user. This packet is isolated, uncommitted source only; the feature has not been enabled by preparing it.

When the rule is off, offer/hire behavior is unchanged. When enabled, every normal liaison's new Journeyman II offers and hires require `Client::GetSodJourneymanTierState(2) == 1`. Identification uses the server template's `ProficiencyID == 2 && TierID == 2`, not client text, displayed grade, class or guessed template IDs. Apprentice, J1 and J3–V follow their existing policies. Higher tiers are not inferred from J2, nor retroactively restricted by this first slice. Those still-permissive other tiers are an explicit scoped policy boundary.

The only progression bypass is active server GM mode (`GetGM()`). Elevated account status without active GM mode still uses normal progression. The bypass skips only this new filter: all existing global/zone mercenary rules, client-version/template checks, shroud handling, raid/group/combat checks, merchant class/range/offered-template validation, contract capacity and cost checks continue to apply. Existing administrative command/ownership-transfer paths are untouched.

## Exact resolver contract owned by Spells

Required public API already supplied in r4:

```cpp
int Client::GetSodJourneymanTierState(uint32 tier);
```

For `tier == 2`, 1 means positively complete; 0 means incomplete; -1 means unavailable/unknown. Every value other than 1 denies a new ordinary J2 acquisition. The consumer makes one lazy read per offer-list evaluation if any J2 template exists, and a fresh evaluation at hire time. No history result is cached between shop and hire, between characters or across sessions. A database/read failure at hire therefore cannot reuse a previous successful shop result.

Spells owns committed native history/archive reads, the feature rule and this API. The bound Oceangreen GROUP tasks are 920177 Familiarity is Key, 920178 Stop the Contamination, 920179 A Dread Challenger and 920180 Beat the Blackburrow Boss. All four personal group-theme completions are needed. Sergeant requester tasks are not personal theme requirements. Raid completion, reward delivery/claim status, other themes, zone access, temporary in-memory task progress, hails and account privileges are not substituted for this result. No duplicate resolver or new entitlement writer is implemented here.

`zone/client.h` is based on the exact Spells r4 postimage so the resolver declarations remain intact. `zone/merc.cpp` and `zone/client_packet.cpp` are based on the retained Build 4b928a766ce9592c6563ff76d51039dc4ff222db source. Build must reconcile any later serving/integration source changes explicitly. Merge this delta with the current Spells implementation rather than replacing its entire client header with an older file.

## Offer and hire paths

`Client::FilterSodMercenaryOffers(std::list<MercData>&)` filters an already client-version-eligible, server-derived liaison offer list. `Handle_OP_MercenaryDataRequest` applies it before counting/encoding rows, stances and merchant slots; empty grades are removed only when filtering changes the list. `CheckCanHireMerc` applies the same policy to a freshly obtained offer list before accepting the requested template or reaching payment. A direct or forged hire request cannot bypass the UI filter. A refused J2 request uses the existing not-offered response and does not charge or create a contract.

The filter does not add templates that a liaison did not previously offer, grant funds or bypass other restrictions. Owned roster display, spawn, unsuspend, zone following, upkeep, suspension, persistence and transfer remain unchanged. No existing owned J2 is revoked by a failed read, disabled slice or incomplete theme. The accepted resource-bar and level100 changes are retained in their exact preimages.

## Bounded acceptance for combined integration

No compile/runtime PASS is claimed by this source handoff. Build owns compilation; the assigned client tester owns native observations. Reuse existing fixtures, not a new campaign.

- Feature off: existing shop list and hire behavior remain unchanged; the resolver is not called by this consumer.
- Feature on, ordinary player: incomplete/unknown J2 is absent from a normal liaison's offers and a direct hire is refused without money/contract mutation. Other offered tiers remain as before.
- Four committed group tasks complete (including a member without Sergeant requester history): J2 is offered and passes the progression filter, subject to all existing hire conditions. Raid-only credit and pending/uncommitted task progress do not substitute.
- A failed fresh hire-time read after a successful shop read refuses J2. The API failure case may be checked with a focused Build-owned harness rather than manipulating a live database.
- Existing owned J2 continues normal roster/spawn/resume behavior. Active GM mode bypasses this filter only; ordinary elevated status does not.

The API contract is the boundary for any Spells implementation changes. Report final serving revision, rule state and bounded native outcomes separately from source/patch validation. No public push before complete local functionality has been accepted.
