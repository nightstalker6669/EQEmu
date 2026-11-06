# Xorbb checked occurrence operations (source only)

Fixed task3040490, zone753 version3. No new schema, migration, registry, journal, achievement definition or runtime installation. The controllers remain disabled. C++ build and native event acceptance are pending the assigned Build/client owners.

The two proposed Lua bindings return `{ok,status,terminal_ready=false,values={...}}`. `ok=true,status='committed'` means one synchronous checked persistence transaction committed. It is not an award, shared-task completion, world acknowledgement, event ordering barrier or native-spawn transaction.

`eq.xorbb_gems_state(client, occurrence_decimal_string, instance, operation, field)`:

- `initialize`: exact public Shimsham753191 quest context/client identity; instance0. Insert missing explicit zeros for the four fixed failure flags, preserving every pre-init1; initialize the monitor marker in the same transaction. An existing monitor with missing flags is rejected.
- `fail`: only `participant_death`, `trapped_open`, `contestant_killed`, `contestant_attribution_unknown`. Player death requires the actual current quest owner client; NPC failures require exact candidate NPC type/role, immutable occurrence/instance tags and current753/version3 instance. All failure writes use the same occurrence lock as initialization; no void set_data fallback.
- `bind`: exact native shared-task client/member and current mission instance; establish the single immutable instance value. No rebinding an existing different instance.
- `read`: exact current mission instance/member; one complete explicit marker/flag/instance snapshot. Missing, duplicate, expiring, invalid or unreadable state is never clean evidence.

`eq.xorbb_chrom_state(client, chrom_npc, occurrence_decimal_string, instance, operation, field, value)`:

- `chrom_initialize`: initialize all six75/65/55/45/35/25 wave states and marker together; reject existing or partial wave state. Requires a real full-health753966 NPC and client/member. The caller must use only the actual first controller-owned Chrom creation after physical gem delivery. The binding does not reconstruct delivery/event history.
- `chrom_read`: complete snapshot, rejecting malformed state. Current native Chrom quest owner and immutable tags must agree.
- `chrom_reserve`, field`wave75` etc: conditional `armed -> pending`; only the successful committed winner may spawn. A competitor receives `not_armed`, never a second permission to spawn.
- `chrom_slot`, field`wave75_slot1` etc,value native entity ID string: require pending state, an empty slot and the actual spawned native add with the exact candidate type/occurrence/instance/wave/slot tags. Slot IDs are receipts for that immediate spawn, not evidence that an actor survives reload.
- `chrom_done`: require pending and two distinct checked slot receipts. No automatic replay of pending, partial or uncertain native effects.

Every operation rejects a pre-existing DB transaction, requires existing shared_tasks/data_buckets InnoDB tables, uses the same `shared_tasks.id` row lock, checks active task/member and applicable dynamic-zone/instance mapping, rejects duplicate global flat bucket rows without assuming key uniqueness, checks writes and exact postimage, and checks commit. Global buckets are uncached. All callers of these keys must use this contract; legacy void bucket writers would bypass serialization. No nested JSON object or competing per-player ledger is used.

The companion adapter deltas remove all get_data/set_data calls from the two candidate modules, consume checked receipts, preserve readinessfalse and native terminal_readyfalse, and refuse replay after local persistence uncertainty. These are owner-integration payloads; the candidate/runtime quest directories have not been edited or installed.

Remaining essential dependency: a failed death write during a DB outage cannot durably invalidate an occurrence through the failed operation. A later terminal reader after recovery can still see old zeros. The occurrence lock serializes successful calls, but cannot establish that an actual cross-zone death event was delivered before a terminal event. A local uncertainty table cannot prove this across another process/reload. No readiness or terminal awards may be enabled until the controller/event owner supplies a conservative occurrence-wide failure policy and a concrete event/terminal ordering guarantee. This patch deliberately returns terminal_readyfalse unconditionally. No generic ledger, new journal or speculative global atomicity is proposed as a substitute.

Verification: source/API and state-transition inspection; exact preimage and generated patch/manifest hashes; both isolated adapters parse with the installed Lua5.4 syntax parser only. No C++ compile, runtime query/mutation, client, native mission or spawn test was performed. Lua5.4 parsing is not target-version or native acceptance.
