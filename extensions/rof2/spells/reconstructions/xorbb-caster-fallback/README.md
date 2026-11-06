# Xorbb: selected class fallback for 24 existing casters

SOURCE ONLY. No database, runtime or client changes. This selects a configurable class fallback for the exact24assigned NPCs; it is **not recovered original RoF spell membership or native casting acceptance**. NPC ownership and any later joint application stay with the NPC owner through JARVIS. No standalone installer or automatic migration is supplied.

| Existing class | Actors | Selected NPC AI list | Eligible entries |
|---|---:|---|---:|
| Necromancer11 |6 | Existing3, unchanged |22 |
| Shaman10 |9 | Existing6, unchanged |21 |
| Magician13 |1 | Existing4, unchanged |10 |
| Wizard12 |8 | Proposed isolated925000, parent0 |17 |

`data.json` contains every selected actor binding, the complete new AI header and17entries, source-entry mapping and held historical exceptions. All24binding changes are restricted to npc_spells_id, expected from0; classes, levels99–102, mana, stats, loot, faction, abilities, paths/spawns and quest roles remain owned/preserved by the NPC owner. Existing Achievements solution augmentation must be preserved when integrating these bindings and on normal spawn. No actor is silently reclassified or given player AA work.

## Wizard isolation

The original default Wizard list2 has19eligible entries, including sword summons10840/5460 tagged as Nuke1. Source inspection found a repeat-attempt risk once a pet exists. Simply changing these to Pet32 would not be a correct repair: their native pet records use targetlock control5 and require an existing enemy target, while automatic Pet32 casting is in the idle path. These are not ordinary idle permanent-pet spells.

The selected fallback therefore uses a separate parent0list containing the other17already-reviewed active entries. Only entry IDs and owning list ID change; every retained spell ID/type, level bound, mana cost, recast, priority, resist/HP/content filter field stays exactly as in the reviewed source list. Shared list2 and all pet definitions stay untouched. The two combat sword entries are omitted from this provisional NPC kit; their original Xorbb combat-pet behavior remains unresolved. No arbitrary level ceiling, Lua pet workaround or new casting engine is introduced.

Proposed server AI list925000 and entry IDs925001–925017 were provisionally reserved through JARVIS and checked for exact namespace/direct-consumer absence. They are server list/entry identifiers, **not new client spell IDs**. No spell-definition remap or extension of a client spell-ID limit occurs.

## Reused compatibility and mana findings

This proposal reuses the completed66unique spell-definition and pet dependency review, rather than repeating it. All four base lists have the same eligible set throughout levels99–102. The kit is an older existing default spellbook, not an assertion that original level100NPCs cast level67–74player spells. Timing, heal strength and exact named role membership remain configurable reconstruction choices.

Native configured mana0 invokes the INT/WIS-and-level formula, with roughly19–20kbase mana for these actors before runtime bonuses/scaling. It does not justify rewriting mana or regen. AI entry manacost-1 uses the actual spell mana cost, not a free-cast workaround. Zero list timing/chance overrides reuse the existing server AI rules. No new rates or power values are fabricated as historical facts.

## Integration guards

`guards.json` provides exact selected bindings, retained default-list metadata and finite namespace queries. `namespace-preimage.json` is point-in-time absence evidence, not application authority. NPC owner integration must obtain current full actor preimages, preserve its combat-profile fields and other-owner augmentation, and use one guarded recoverable transaction for18newheader/entryrows plus24binding updates. Publication and separate coordinated application authorization precede mutation. No new spell or pet definition is required, and shared lists3/4/6remain unchanged.

All24actors have a selected fallback; the held exceptions are historical membership/cadence/balance and original Wizard combat-pet behavior, not silently skipped actor bindings. Root may mark the source dependency prepared after reviewing this packet, but it must not call the NPC AI installed or natively accepted before actual application/readback and separately authorized validation.
