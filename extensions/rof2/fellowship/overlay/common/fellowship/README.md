# Native RoF2 fellowships

This implementation targets the May 10, 2013 RoF2 client. Install the matching
RoF2 opcode mappings and updated world/zone binaries, and apply these additive,
repeatable migrations to the character database in order:

1. `utils/sql/fellowship/001_fellowships.sql` — membership, invitations, campfires
   and the shared mutation guard.
2. `utils/sql/fellowship/002_vitality.sql` — character-owned vitality banks and
   observed capacities.
3. `utils/sql/fellowship/003_xp_commits.sql` — atomic combat-XP audit receipts.
4. `utils/sql/fellowship/004_campsite_catalogue.sql` — all 597 campsite definitions,
   material identities and documented effect policies.

The participating inventory, character, fellowship and journal tables must use
InnoDB. Partial/unavailable schema or an uncertain transaction cannot authorize
an inventory debit or a fellowship XP award. Other clients do not acquire native
fellowship windows from these changes; older clients receive a text fallback
for fellowship chat.

## Membership and native UI

Implemented paths include creation, invitations, twelve-member capacity, leader
transfer, removal, leaving/disbanding, MOTD, channel-21 chat, sharing preferences,
native vitality gauges, campsite management and insignia travel. Zone entry and
cross-zone invalidation reload authoritative membership. Character deletion
removes invitations/membership and promotes the earliest surviving member or
removes an empty fellowship.

Inviting requires a nearby targeted character and expires after five minutes.
The native accept/decline packet has no invitation identity, so the server binds
it to the inviter and expiry actually displayed to that authenticated session.
Competing invitations cannot replace a live offer; retries preserve its expiry.
Authority comes from the authenticated character and locked database state,
never client-declared member or fellowship identities. Chat authenticates the
originating zone/character and rechecks recipient membership before delivery.

## Campsites and materials

All 597 catalogue entries are implemented: the free basic campfire and 596 paid
variants covering spell focus, melee focus, statistics, resistances, illusions
and support buffs. Native category filtering follows the matching client's
`Resources/campsitecategories.txt`. The basic Fellowship of Honor entry uses
server ID 1; paid IDs use their recovered description IDs. This ID allocation is
server policy, not a recovered Live campsite-ID table.

Creating a campsite requires three living, unengaged fellowship members within
100 units. Each paid recipe consumes one exact kit and one exact lumber bundle
from the creator's personal inventory. Creation and both inventory decrements
commit together. The debit verifies current membership, the held inventory
session, exact persisted/runtime item identity, stack quantities and pending
inventory/coin state. An uncertain commit requires authoritative inventory
reload rather than retrying the purchase. The basic campfire requires no items.
Campsites expire after one hour. Insignia travel retains ordinary zone and
instance admission checks.

A campsite has a visible world entity. Its effects apply to nearby living
members and are reconciled on membership/campsite changes and during the zone
heartbeat. All 304 numeric/melee variants project through 27 existing native
Standard buff templates: 7354–7371 and 7607–7615. Their effective caster levels
are derived from the primary formulas to match the described amount or the
explicit melee-tier policy. The complete twelve-slot effect signatures and
buff duration/song fields must match the primary template data before these
variants can be created or projected.

The real buff is the sole authority for server bonuses and native stat display;
there is no parallel manual addition to `spellbonuses`. Ordinary spell stacking,
stat caps, haste and damage-shield rules apply. Before applying a lease, the
server refuses any stacking outcome that would overwrite an ordinary buff.
A conflicting ordinary buff may therefore prevent the campsite buff until the
conflict ends. The short renewable leases carry a reserved caster marker,
are omitted from saved buffs, and legacy marked rows are discarded on load.
The `OP_Buff` projection carries the effective caster level and refreshed
duration so the client computes the same template bonus as the server.

Focus retains its explicit manual maximum policy and competes with the existing
spell-focus source while preserving negative-focus precedence. Illusions use
existing Standard appearance data rather than inventing spell IDs, and yield
to ordinary illusion/illusion-copy buffs. Support uses Standard of Faerune/Dead
Man spells 7616/7617 under the same scoped lease ownership; ordinary copies from
another caster retain their own ownership and persistence.

Primary spell data establishes these melee effect templates:

| Family | Spell | Effect | Formula coefficient | Maximum | Skill restriction |
|---|---:|---|---:|---:|---|
| Cleave | 7354 | CriticalHitChance, SPA 169 | 6 | 48 | 0, 1, 2, 3, 8, 10, 26, 28, 30, 36, 74, 77 |
| Ferocity | 7355 | DoubleAttackChance, SPA 177 | 1 | 8 | Normal double-attack calculation |
| Sharpshooting | 7356 | HitChance, SPA 184 | 1 | 8 | Archery 7 and throwing 51 |
| Evasion | 7357 | DodgeChance, SPA 174 | 3 | 24 | Normal dodge calculation |
| Prevention | 7358 | ParryChance, SPA 175 | 2 | 16 | Parry; no block effect |

These coefficients/caps and skill masks are recovered from the matching shipped
`spells_us.txt`. The recipe-to-effective-caster-level mapping is not recovered.
For the 40 yellow-lumber melee variants, the explicit emulator policy assigns
Elm, Alder, Maple, Ash, Fir, Briarwood, Oak and Teak effective tiers 1–8, then
applies the recovered coefficient and cap. This is not a claim of exact Live
conversion. Likewise, default effect radius 100 and deterministic maximum focus values
are explicit emulator policies where primary evidence does not establish the
Live radius or random focus distribution. Other numeric catalogue values come
from matching-client descriptions.

The persisted server expiry serves as the native nonzero campfire-presence
marker. The inspected window consumes presence and zone information; the
original clock origin and complete native timer semantics remain unconfirmed.

## Vitality and combat XP

Original combat-earned normal/AA XP is divided among opted-in members, including
offline members; the donor retains division remainder. Existing banked vitality
can provide up to four additional normal XP or two additional AA XP per retained
XP, producing up to 5x/3x totals. Bonus XP is never shared again. Banks belong to
the character and survive leaving or disbanding.

Normal bank capacity is ten current-level XP ranges, bounded by remaining
configured progression. AA capacity is bounded by remaining unused-AA
progression and the safe signed storage limit. Existing per-kill normal/AA caps
also constrain the resulting award. Known full banks or unavailable normal
progression stop sharing; offline members retain their last observed capacities.
Members without observed capacities use bounded safety ceilings until refreshed.
Cross-level distribution, AA conversion and cap calculations are documented
emulator policies, not established Live-equivalent conversions.

Bank changes, the final character XP/AA/progression values and an audit receipt
commit on the same verified connection and transaction. The journal retains the
latest 256 receipts per character; it is an audit history, not a replay queue.
There is no separate character-save crash window between a committed fellowship
bank mutation and its committed XP award. If the outcome becomes uncertain after
runtime XP changes, further saving is blocked and the character reconnects to
reload the authoritative result. An uncertain award is never blindly retried.

## Housing and validation

Personal-property `_Fellowship` ACL entries resolve the current owner's and
actor's actual fellowship memberships inside the housing transaction. The same
mutation guard prevents leave/remove/disband from racing an in-flight access
check. Named grants retain native precedence; inventory ownership, session
fences, rent and capability checks remain separate. Guild properties do not
borrow a guild leader's fellowship. Metadata, scenes, item operations and
upkeep withdrawals use the current typed access checks. Membership changes
refresh housing catalogues/scenes locally and across zones.

A physical paid-HP check exposed the earlier manual-field approach: that field
was not used by `Client::CalcMaxHP` and supplied no native buff projection.
Health description 41586 now uses Standard of Health 7607 at effective level 2;
its SPA 69/formula 75 yields 150 HP through the normal spell/client calculation.
The rebuilt and deployed server passed the native repeat: client maxHP2374 to2524
and server2671 to2821, with the150HP removed on camp destruction. Three-client
acceptance also covered paid material debits, Faerune, barbarian appearance and
expiry restoration, category filtering, cross-zone chat, insignia travel, normal
and AA sharing/redemption, offline banking, restart/relog persistence, removal
and disband. The first HP failure documents the superseded implementation.

Protocol evidence, fault/concurrency tests, material and XP transaction fixtures,
and native screenshots are retained outside production source under
`eq-rof2-analysis/fellowship-20261023/completion`. Exhaustive catalogue validation
and real database tests complement these sampled native gameplay checks. This
is functional local emulator support; exact Live policy parity remains limited
as described above.
