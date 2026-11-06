# Shared engine: exchange policies, reward identities and shutdown

This source layers the generic engine changes from spell commits `8301c3b78727c48b1d41ce72c7141a5ae6c9f186` and `60c93741e6f91a85f59e15c07ad5438e12408f9b` over shared-core input `17d63f7d512711e911787e630f4533ee14a1e785`. Binary database schema is9338. No spell or achievement quest content is activated by the shared core. Feature branches retain their own content and must record the core they adopt.

## Prerequisite schema9336 and generic spell behavior

Schema9336 adds merchant language requirements with unrestricted defaults. Listing and purchase paths validate the configured language and skill; invalid requirements fail closed. Ground-ring persistent auras use the selected point, validate finite coordinates/range and apply detrimental line-of-sight checks. They create one caster-owned persistent entity instead of one aura per nearby recipient. These mechanisms contain no quest, spell or item IDs.

[Installation](INSTALL.md#schema-93369338) covers the ordered update and database ownership.

## Personal exchange policy

Migration 9337 adds the content-only `task_spell_exchange_policies` table. Registered personal tasks can specify original input-item refunds, language/faction admission and learned-spell hiding. Policy loading validates complete unit-item options and the unrestricted original-item refund; incompatible or shared registered tasks fail validation. Unregistered tasks retain their existing behavior. Admission is checked on both task listing paths and direct acceptance. Permanent class learning state is used for spells and disciplines. A multi-spell reward is hidden only when every ability is learned; its entries are never partially stripped.

Migration 9338 pins the hide-known flag per task reward occurrence, default0 for existing occurrences. Display, pending restoration and unlocked claim use the same policy. A selected choice remains immutable for ledger retries. Shared tasks and unregistered personal tasks retain default0. Existing migration IDs9331–9336 and their original SQL are unchanged.

## Durable wire identities

All providers share one independently committed allocator. A new catalogue or semantically changed option receives fresh IDs; unchanged options can retain their token while a changed visible catalogue rotates its set ID. Comparison includes the complete reward graph, labels, flags, ordering and class policy. Removed option tokens cannot be rebound after refresh, reopen, reconnect or process restart. ID exhaustion refuses a new display without wrapping.

The new `reward_selection_wire_allocator` is server state, separate from character copy/reset tables. Never truncate/reset/decrease its high-water mark. Preserve the largest issued `last_id` across older snapshot restoration. Content rollback and grant rollback must not recycle published tokens. Before restoring an older database, preserve the greatest issued high-water mark and reinstate at least that value before services resume. Drain legacy pre-allocator reward sessions and restart clients during the initial transition; old reusable IDs are outside the new durable namespace guarantee. The allocator uses a separate autocommit database connection so caller transactions cannot roll its reservations back.

## Event-loop lifetime correction

The shutdown baseline showed libuv use-after-free because thread-local loop destruction precedes static service destruction. Timers, servers, TCP connections/pending connects, DNS requests, queued work and reliable UDP handles now register cleanup with their owning loop. Teardown closes/cancels resources while owners remain alive and drains deferred callbacks before freeing the loop. Callbacks do not revive services during thread shutdown. A late stream Close after UDP detachment is guarded before allocating/sending a packet. Normal thread-local loop ownership is retained. The configuration-check override also has an initialized false default and is parsed on each configuration load.

## Validation and limits

* The source owner recorded combined world/zone links. The complete shared-core source also passed an independent clean GCC12/C++20 world/zone build with normal CMake/PCH settings, serial compilation, a read-only dependency cache and a resource-limited network-disabled container. The uninitialized stock websocketpp submodule found in the first attempt was populated at its exact upstream pin before resuming; no engine source correction was required.
* 277 pure policy/wire cases pass: class masks, learned bundles, retained refunds, disabled legacy policy without item lookups, invalid snapshots, selected retries, full semantic changes, retired/re-added tokens, reopen, exhaustion and invalid prior set IDs.
* Isolated MariaDB migration checks preserve legacy occurrences and defaults, retain rows/counters on rerun, reject an incompatible state column, and allocate360 disjoint committed ranges through12 concurrent workers. Independent caller rollback cannot recycle IDs; exhaustion refuses wrapping.
* The original world shutdown fault was reproduced under Valgrind. The repaired isolated world passed five connected normal shutdowns and connected/unconnected Valgrind runs with zero errors. Standalone timer/thread/pending-work teardown also reported zero errors. A late UDP Close reproduced a null-handle send before its guard; after the guard Valgrind reports zero errors and all heap blocks freed.

The complete generic extraction applies cleanly to both recorded achievement and integration input trees, preserving their feature-only patches. Registered manifest9331–9338 entries and original migration SQL retain their source bytes. No opcode mapping or packet structure is added; `MerchantList` language fields are server catalogue metadata.

Private harnesses, logs, database snapshots, binaries, credentials and client assets are excluded from this production branch. This is offline evidence. Native-client exchange, reconnect and shared-task regression acceptance remain required in a coordinated maintenance/test window. The full world Valgrind checks do not establish that unrelated process leaks have been eliminated.
