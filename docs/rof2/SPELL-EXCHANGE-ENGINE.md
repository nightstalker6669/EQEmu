# Offline spell-exchange engine extension

This delta extends the pinned shared-core base `17d63f7d512711e911787e630f4533ee14a1e785`. The achievements maintainer owns review, extraction and publication to `rof2-shared-core`. The spell branch retains the base pin until that extraction is published. No achievement content, client session, production database or running binary was changed by this offline work.

## Personal exchange policy

Migration 9337 adds the content-only `task_spell_exchange_policies` table. Registered personal tasks can specify original input-item refunds, language/faction admission and learned-spell hiding. Policy loading validates complete unit-item options and the unrestricted original-item refund; incompatible or shared registered tasks fail validation. Unregistered tasks retain their existing behavior. Admission is checked on both task listing paths and direct acceptance. Permanent class learning state is used for spells and disciplines. A multi-spell reward is hidden only when every ability is learned; its entries are never partially stripped.

Migration 9338 pins the hide-known flag per task reward occurrence, default0 for existing occurrences. Display, pending restoration and unlocked claim use the same policy. A selected choice remains immutable for ledger retries. Shared tasks and unregistered personal tasks retain default0. Existing migration IDs9331–9336 and their original SQL are unchanged.

## Durable wire identities

All providers share one independently committed allocator. A new catalogue or semantically changed option receives fresh IDs; unchanged options can retain their token while a changed visible catalogue rotates its set ID. Comparison includes the complete reward graph, labels, flags, ordering and class policy. Removed option tokens cannot be rebound after refresh, reopen, reconnect or process restart. ID exhaustion refuses a new display without wrapping.

The new `reward_selection_wire_allocator` is server state, separate from character copy/reset tables. Never truncate/reset/decrease its high-water mark. Preserve the largest issued `last_id` across older snapshot restoration. Content rollback and grant rollback must not recycle published tokens. The allocator uses a separate autocommit database connection so caller transactions cannot roll its reservations back.

## Event-loop lifetime correction

The shutdown baseline showed libuv use-after-free because thread-local loop destruction precedes static service destruction. Timers, servers, TCP connections/pending connects, DNS requests, queued work and reliable UDP handles now register cleanup with their owning loop. Teardown closes/cancels resources while owners remain alive and drains deferred callbacks before freeing the loop. Callbacks do not revive services during thread shutdown. A late stream Close after UDP detachment is guarded before allocating/sending a packet. Normal thread-local loop ownership is retained. The configuration-check override also has an initialized false default and is parsed on each configuration load.

## Validation and limits

* The complete combined source links world and zone using GCC12/C++20 and the standard CMake/PCH build in a resource-limited network-disabled container.
* 277 pure policy/wire cases pass: class masks, learned bundles, retained refunds, disabled legacy policy without item lookups, invalid snapshots, selected retries, full semantic changes, retired/re-added tokens, reopen, exhaustion and invalid prior set IDs.
* Isolated MariaDB migration checks preserve legacy occurrences and defaults, retain rows/counters on rerun, reject an incompatible state column, and allocate360 disjoint committed ranges through12 concurrent workers. Independent caller rollback cannot recycle IDs; exhaustion refuses wrapping.
* The original world shutdown fault was reproduced under Valgrind. The repaired isolated world passed five connected normal shutdowns and connected/unconnected Valgrind runs with zero errors. Standalone timer/thread/pending-work teardown also reported zero errors. A late UDP Close reproduced a null-handle send before its guard; after the guard Valgrind reports zero errors and all heap blocks freed.

Private harnesses, logs, database snapshots, binaries, credentials and client assets are excluded from this production branch. This is offline evidence. Native-client exchange, reconnect and shared-task regression acceptance remain required in a coordinated maintenance/test window. The full world Valgrind checks do not establish that unrelated process leaks have been eliminated.
