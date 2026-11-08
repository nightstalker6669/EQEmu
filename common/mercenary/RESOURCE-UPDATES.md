# Mercenary group resource updates

Mercenaries now check group mana and endurance percentages independently on the existing two-second NPC HP refresh timer. Before this fix, `Merc::Process` only called `SendHPUpdate`; its unchanged-HP early return prevented `Group::SendHPPacketsFrom` from sending the accompanying resource percentages. A healthy mercenary could spend or regenerate resources while the group window remained stale.

Changed percentages, including zero and full, are sent through the existing group broadcast helpers to other SoD-or-newer group clients. Cached percentages suppress unchanged periodic broadcasts. A new group ID forces both resource snapshots; ungrouping resets the cache. Existing join snapshots cover clients joining an unchanged group. New mercenary instances start with invalid caches. HP target, extended-target and group delivery remain in `Mob::SendHPUpdate`. Resource changes later in the same process iteration are observed by the next refresh.

Zero-capacity mana pools are represented as zero in all three group snapshot/update paths, and the mercenary endurance percentage getter handles a zero-capacity pool. Endurance packet allocation names its own structure (the existing structures are both three bytes).

## RoF2 wire route

No new opcode or encoder is needed. `patch_RoF2.conf` maps `OP_MobManaUpdate` to `0x2404` and `OP_MobEnduranceUpdate` to `0x1c81`. Both use the existing packed three-byte shared layout: little-endian `uint16 spawn_id` at offset 0 and `uint8` percentage at offset 2. The RoF2 strategy defaults to passthrough; neither opcode has an overriding encoder. These percentage packets differ from the full current/max `OP_ManaUpdate` and `OP_EnduranceUpdate` encoders. This is source routing evidence, not fresh native consumer validation.

## Regeneration and scope

Resource spending, buff upkeep, base regeneration, meditation, rest delay and rest regeneration formulas are unchanged. `Merc::Process` already changes the authoritative pools on its tic timer; eligible sitting, out-of-combat rest recovery is computed by `CalcRestState`. A stale displayed bar cannot establish whether those pools are actually recovering. This patch adds notifications only and retains the level-86–100 content and discipline fixes. It has no SQL migration.

## Focused acceptance handoff

Build and client acceptance are pending. Reuse one assigned isolated client and existing fixtures; no new accounts, manual leveling or broad test campaign is required.

1. With mercenary HP stable, observe authoritative mana/current maximum and the group bar before and after a completed paid cast. Within the next two-second refresh, the group percentage should reflect the actual pool independently of HP.
2. While eligible for sitting/out-of-combat rest, compare authoritative mana and endurance recovery with the group percentages through zero/partial/full values. Measure rates separately; this change makes no rate claim.
3. For a melee discipline/upkeep spend, compare authoritative endurance with the group bar while HP remains stable. Confirm an ordinary HP loss/heal still updates the group and target displays.
4. Reuse a normal join or suspend/resume transition to confirm the current resource snapshot. If another group client is already available, confirm it receives the same percentages. No additional fixture is required solely for a second viewer.

Record source revision, spawn/group identity, actual current/max values, observed percentages and timestamps. Publication, compilation and deployment are separate from native acceptance; do not label this runtime PASS from static inspection alone.
