# Gameplay policies and content

The twelve-member client roster, message layouts, campsite category IDs, native spell signatures and shipped numeric descriptions are observed evidence. The following choices complete local emulator behavior where evidence does not recover Live semantics.

| Area | Implemented policy | Evidence boundary |
|---|---|---|
| Invitations | Five-minute, delivered-session-bound offer; one live offer per recipient | Native response has no nonce/inviter identity |
| Camp creation | Three living unengaged members within100 units; one active camp | Historical description establishes proximity/three members; exact radius is policy |
| Camp lifetime |3600 seconds | Historical fellowship description; service safety upper bound14400 is not UI lifetime |
| Camp effects | Radius100, living eligible members in matching zone/instance | Exact Live radius unproven |
| Recipe IDs | Basic ID1; paid IDs equal description IDs | Server allocation, not recovered Live kit-ID table |
| Numeric/melee buffs | Real Standard spell at derived effective level; ordinary stacking | Exact numeric descriptions matched; melee recipe-level association is policy |
| Focus | Deterministic described maximum; competes with spell focus, ordinary item/AA contribution retained | Exact Live random distribution unproven |
| Melee yellow lumber | Elm/Alder/Maple/Ash/Fir/Briarwood/Oak/Teak -> effective levels1..8 | Native formulas proven; mapping policy |
| XP donation | Original normal and AA XP split separately among eligible opted-in members; donor keeps remainder | Cross-level and normal/AA conversion not recovered |
| Redemption | Up to4 additional normal or2 additional AA per retained XP, respecting room/per-kill caps |5x/3x totals supported historically; exact conversion remains policy |
| Normal bank capacity | Ten current-level XP ranges bounded by progression | Exact Live level-dependent formula not fully recovered |
| Offline capacity | Last observed caps, otherwise normal1e12/AA2e9 safety ceiling | Bounded emulator fallback |

All597 entries are enabled in the compiled catalogue: one free basic plus596 paid recipes, with71 paid families and149 unique required materials. Each paid recipe consumes one kit and one lumber; availability depends on current materials, nearby members, no existing camp and valid native templates.

Category mask is `1u << category_id`:1 General,2 Spell Focus,3 Melee Focus,4 Statistics,5 Resistances,6 Illusions,7 Buffs. `0xffffffff` requests all categories. The native ShowAll flag reverses the request's usable-only word.

Numeric/melee variants use27 Standard spells7354–7371 and7607–7615. For example health description41586 uses Standard of Health 7607 at level 2: primary slot11 SPA69/formula75 produces150HP. The real scoped buff is the sole numeric server/client authority. No extra manual `spellbonuses` addition is applied. The effective caster level is included in OP_Buff so both sides compute the same value.

Melee formulas: Cleave7354/SPA169 coefficient6 maximum48, twelve native melee skills; Ferocity7355/SPA177 coefficient1 maximum8; Sharpshooting7356/SPA184 coefficient1 maximum8, archery/throwing; Evasion7357/SPA174 coefficient3 maximum24; Prevention7358/SPA175 coefficient2 maximum16, **parry**. Prevention does not grant block. All twelve-slot/duration/song signatures must match.

Buffs use two-tick renewable leases marked `Fellowship_Campsite_Aura`. Ordinary copies remain separate; applying a campsite cannot overwrite an ordinary buff, even a weaker one. Scoped leases are omitted from saved buffs and reconstructed from authoritative position/camp on relog. A normal conflict can leave an otherwise valid camp with no bonus for that member until the conflict ends.

Illusions use only native appearance data, yield to normal illusion/illusion-copy buffs and player blocking, and restore prior appearance while respecting later external cosmetic/size changes. Support uses Standard7616 Faerune and7617 DeadManFloating; no fictional spell IDs are added.

Vitality belongs to the character and survives leaving/disbanding. Shrinking a capacity does not erase an existing pool. Full banks auto-disable sharing and are excluded before donation division. Bonus is not redonated. Custom quest code that independently commits external effects cannot be made atomic by the fellowship finalizer; see [DATABASE.md](DATABASE.md).
