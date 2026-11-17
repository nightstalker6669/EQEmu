# Rathe-group Journeyman V acquisition consumer

This narrow extension uses `SodProgression:EnableRatheSlice`, declared default false. It extends the existing independent J2/J3/J4 offer/hire filter to J5 without changing the call sites, other theme rules or existing owned mercenaries. Only server templates with `ProficiencyID==2 && TierID==5` are newly governed when the Rathe rule is on; liaison/client compatibility, class, cost, capacity, zone/combat and other native restrictions still apply. All existing races/classes/templates are reused; there is no SQL/template grant or contract deletion.

A fourth lazy cache slot holds the result of the existing `Client::GetSodJourneymanTierState(5)` for that offer-list evaluation. Only result1 (positively complete) allows new ordinary J5 acquisition;0 incomplete/-1 unknown/any other result refuses. The shop uses the filter before encoding counts/rows; authoritative hire independently repeats it before payment/contract mutation. A shop response is not a cached entitlement. Source-supported theme arithmetic already maps tier5 to independent `Evidence.group[3]` (Rathe). No lower theme or raid credit is synthesized.

Spells has implemented the authoritative binding in `ReadEnabledGroupSlices`: each character must have **all seven distinct exact-ID/title acknowledged native completions** below. The six personal completions and Murdunk shared-member archive feed independent `Evidence.group[3]` (Rathe), which the existing tier-state resolver exposes as J5 entitlement. The binding uses Database reservation r2 SHA256 `0eda1c523a5632af635c1d3d2860ba0836300a8d029eb4ec1ee4397d7f3b43b5`.

| Task ID | Exact title | Native completion evidence |
| --- | --- | --- |
| 920216 | Retinue of the Frog-God | Personal completion record (`activityid=-1`) |
| 920217 | Find Your Allies | Personal completion record (`activityid=-1`) |
| 920218 | Whom Frogs Destroy | Personal completion record (`activityid=-1`) |
| 920219 | The Wrath of Korascian | Personal completion record (`activityid=-1`) |
| 920220 | Disrupt the Siege | Personal completion record (`activityid=-1`) |
| 920221 | Champions No More | Personal completion record (`activityid=-1`) |
| 920222 | Murdunk's Last Stand | Acknowledged shared completion/member archive |

Murdunk alone does not complete the Rathe theme. Joining Murdunk does not require the requester's personal prerequisite chain; earning J5 still requires that character's own six personal completions plus acknowledged Murdunk membership. Task acceptance, a hail, partial objectives, raid completion, reward delivery/claim status and GM status do not substitute for durable completion. The consumer adds no history writer, entitlement flag or reward-claim proxy.

The native test confirmed an ordinary peer earned all seven completions and hired the existing Journeyman V healer (template 659). The offer and hire succeeded under normal mercenary checks, and the peer kept existing contracts. One exact shocktrooper test spawn assisted the peer's Disrupt the Siege completion. The spawn was defeated for normal task credit.

Active character GM mode retains the administrative filter bypass. Earned verification used a peer outside GM mode.
