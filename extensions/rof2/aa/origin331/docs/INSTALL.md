# Origin 331 integration source

This package preserves the reviewed Origin-only implementation. It is not a standalone change that compiles on the publication branch. The four native files on `rof2-aa` remain unchanged. No full integrated source or global player script belongs in this publication.

## Required integration

1. The coordinator selects an integrated source target containing the real shroud and fellowship persistence state guards listed in the manifest. Published companion pins establish symbol provenance only. They do not certify selective integration or authorize importing the whole companion feature. Never replace these guards with stubs or remove them.
2. Match the four target preimage hashes in the manifest before applying `native-integration.patch`. If they differ, reconcile only the Origin delta with the source owners, including the Merc header overlap, and issue new exact hashes. Do not overwrite full native files.
3. Build and deploy that reviewed actual integration through Build ownership. This package has not been compiled, deployed, or validated in a client. No database migration, player ownership mutation, or generic SaveAA rewrite is supplied.
4. After the matching `ReconcileOriginEntitlement` native Lua binding is deployed, match the global script preimage and apply `quest-overlay/global_player.origin.patch` to the quest root. Install the separate helper at `lua_modules/aa_origin_entitlement.lua`. The global patch does not create the helper. Never activate the hooks with an older binary. The patch retains existing callbacks and their nonzero returns, removes the unconditional level-five success popup, and announces success only after the native durable receipt.

## Rollback

Use `native-rollback.patch` only against its exact postimages and through source/build ownership. Reverse the global context patch only against its exact postimage, preserve unrelated callbacks, and remove the helper only when its hash matches this manifest. Coordinate native and quest rollback so no active hook calls a missing binding. Player AA ownership is not deleted by source rollback.

## Scope and limits

Only Origin ability 331, first rank 1000, level 5, cost 0, spell 5824, reuse 1080 is accepted. The native method checks actual eligibility and runtime state, writes only the unowned Origin row on a private existing database connection with query replay disabled, and confirms committed rank 1 / charges 0 before publishing memory and UI. Existing owners are unchanged. Failed acknowledgement is reconciled by a later ordinary eligible event, with no zone freeze or unrelated item, task, currency, or AA-point mutation.

Exact source-target preimage hashes are in `sanitized-source-manifest.json`; full preimages and host runtime paths remain private. Prior validation was patch applicability against those exact integrated preimages, static review, and Lua syntax. Publication-branch compilation, runtime installation and native acceptance remain unperformed.
