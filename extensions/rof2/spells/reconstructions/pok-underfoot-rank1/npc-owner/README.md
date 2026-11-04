# Reserved PoK81 provider source payload

2026-11-04. Source preparation complete; **never applied, invoked or registered**. Contains12npc_types,12spawngroup,12spawnentry and12spawn2rows. Dedicated merchant IDs equal NPCIDs202900–202911; group/spawn IDs304050000–304050011 follow Spells' exact missing_selected_names order in selected15-provider-preimages.json. [BINDINGS.md](BINDINGS.md) is the exact stock-owner handoff; 43new-provider stock rows (final Spells named inventory reconciliation) belong to Spells, not this component.

The original three existing actors and placements are preserved: Beorobin202158/spawn40526, Kurlond202288/spawn40620, Heldin202310/spawn40659. Their9stock-only rows are a separate Spells component; no copies, updates, moves or renumbering here. Rogue3remains held.

## Template and historical qualification

Full retained Beorobin202158row is the normal PoK merchant template: class41, level40, HP2600, AC487, damage32–150, regen13, existing merchant protection ability string, no new spell or loot list, findable1. New identities/merchant bindings are explicit overrides; existing template row is not edited. The numeric level/stats/protection/respawn values are **current emulator template defaults**, not recovered2012provider stats; they replace the earlier unselected level60reference candidate. Existing qglobal/interaction and other ordinary template fields are inherited explicitly in payload.json, rather than silently importing a player's ranger/caster class.

Historical names/81+roles,findability/library floors and source /loc come from the reviewed2012oldids recorded per binding. /locY,X,Zis converted to DBX,Y,Zwithout map-axis sign inversion. Candidate source elevations are retained, including3/33/66/69/99and negative basement values; no arbitrary floor snapping or falsely claimed native collision/path proof. Actual geometry remains unaccepted until the coordinator schedules normal validation.

Eleven new appearances use labelled generic human1/gender0/texture0/helm0/size6/face0defaults. Sweeneph202902uses the prior tentative ratman156/gender2candidate informed by an undated source image. None is an original model claim. All heading0values are labelled defaults. Stationarypath0, version0,640secondrespawn/variance0, group singleton behavior are local operational defaults, not proof of original orientation or timer. New12spawn2rows are gated at min_expansion18 for the March2012/VoA merchant-route introduction, with max_expansion-1. Spawnentry min_expansion-1is a neutral child filter, not an earlier-route grant: parent spawn2gate18controls availability. No path, door, existing placement or rare-selection row changes.

## Exact namespace check

collision-check.sql and collision-receipt.json record the only live reads for this assignment: exact proposedNPC/merchant IDs and exact proposedspawn/group IDs, including actor/merchantlist/tempmerchant references, spawnentry, spawn2_disabled and respawn_times. Every queried result was empty. No replacement range required. No broad duplicate actor audit, geometry/runtime probes or native tests were performed. This is point-in-time source-preparation evidence; the installer refuses an intervening exact collision.

## Integration / recoverability contract

providers.sql.disabled defines a source-only guarded component with **no CALL, no migration registration and no independent COMMIT**. The manifest's disabled status is not an invented npc_types.enabled column. Installing the spawn rows would eventually make actors load normally, so do not apply them before their reviewed stock rows and the integrated source have coordinator authorization/publication.

Final integration owner should create any needed stored routine **before** the transaction (routine DDL can implicitly commit), then use one reviewed transaction for these48rows plus Spells'43reviewed new-provider stock rows, and concise readback. The component expects new merchant stock namespaces still empty at its guard, so invoke/inline actor insertion before inserting those new-provider stock rows. Preserve native item base prices; stock qualifications and original mission acquisition remain Spells-owned. Do not execute this component standalone, insert empty merchants live, split commits or infer application authority from provisional reservations. The explicit session guard is source integration scaffolding, not a new user approval workflow.

Backups/preimages: proposed IDs were absent; retained existing3actor/placement preimages are referenced and untouched. Capture exact full postimages for all newly inserted records when an apply is actually authorized. Rollback should refuse if any new row or associated stock/reference has drifted, remove only matching owned new stock/entries/spawns/groups/NPCs in dependency order, and preserve existing3providers/9stock-only rows unless their separate owner supplies a rollback. No unguarded bulk-delete rollback is emitted from source-only evidence. Actual loaded actors/cache/reload cleanup must remain Build/tester-owned, with no claim SQLrollback alone despawns runtime objects.

Source artifact review confirms the12bindings reference 43new-provider stock candidates and align one-to-one across the four tables. SQL has not been run against the database. GitHub chat owns publication; coordinator controls any later application. No Chrome work was needed for this assignment; prior research references are reused.

## Era correction — 2026-11-04

Coordinator relays Spells: the twelve new spawn2rows require min_expansion18, reflecting the2012/VoA merchant route even though the spell identities originate in Underfoot. This source correction changes only those twelve parent spawn gates. Spells separately gates all54newstockrows at18 (43on new merchants plus9stock-only rows for the original three and2existing-tome additions). Preserve the existing three actors/spawns and all prior stock availability; Kurlond's older min7does not establish availability for this newly added route. Existing122stock baseline/sole-consumer evidence is owned and captured by Spells; no repeated scan. `era-correction-receipt.json` retains the source preimage and qualification. No source fragment was invoked or applied.

## Stock-only binding correction — 2026-11-04

Spells assigns Honorific Mantle119709/spell22665to existing Ulin Velnik202161/merchant202161, using public spell22665SoldBy/NPC26168 evidence. External26168is a source key, never a local allocation. CavalierTummings202901retains three PALspell candidates; no119709binding to that new merchant. New-provider stock total is now **at most44**, not a final audited44-row guarantee; Spells is finishing named inventory audit. Ulin's1stock-only row is separate, the existing9component is unchanged, and Rogue3held. This correction changes stock references/counts/comments only; all48actor/group/entry/spawn source rows remain unchanged. No profile/preimage/collision checks repeated and no DBapply.

## Final stock reconciliation — supersedes prior provisional44count

Spells' final named-stock audit selects43rows on the twelve new providers: Cavalier202901andReaver202903eachretain3. Honorific Mantle119709/22665belongs to existingUlin202161merchant202161; Malarian Mantle119633/22662belongs to existingZhaoV`karin, public sourceNPC13960. Exact Zhao nativeID/merchant/preimage is pending fromSpells and deliberately not guessed here. The two separately guarded existing-tome additions plus originalexisting9remain Spells-owned; Rogue3held. All48actor/group/entry/spawn source rows remain unchanged. Only stock references/counts/comments updated; no profile/absence audit or live application. Final integrated manifest belongs toSpells.
