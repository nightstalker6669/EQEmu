# Fire personal successor acceptance bridge, revision 2

This integration-source package preserves the exact reviewed four-file runtime patch. It is uncompiled and does not activate the whole Fire feature. It adds artifact files only; raw native branch files are unchanged.

## Target and prerequisites

The reviewed runtime preimage of `zone/task_client_state.cpp` is pinned in `source-manifest.json`, with three header paths required absent. The runtime checkout HEAD is provenance only: its dirty source preimage is authoritative. The published `rof2-shared-core` baseline at `2d05521f1c0ae819bea55f416e82e07d40b610e0` instead has SHA256 `f412c63281a97d2b7dd6ccf54b87ba844b30532ddbe3519644bc95b706506e3c` for that file, differing from the required `1b25852d7d2011a0669dab8108e7026ad6cc37422ef0783223d1d45b9f1c3c14`. The three header paths are absent there. This package is not a certified standalone integration against that branch.

The native task reward occurrence/reservation and persisted reward ledger strata, actual bool `SaveClientState` result, task and activity structures, and original physical-finish R2 handoff owner must match the reviewed runtime context. Existing branch names or similar APIs do not establish this match. Reconcile dependencies in a separately authorized source change; do not import whole integrated runtime files, remove guards, or invent adapters to make the patch apply.

Revision 2 changes the already-engaged optional reservation assignment to `*occurrence_id`. The patch bytes are pinned by SHA256 `b71dc874870e6c0ab0ac78f85536c75913da594089ae4a18b3aa9e7cdd976764`.

## Actual data flow

The feature-private `DispatchActualOriginalPersonalSuccessorOnce` checks the actual original Gotonur NPC, owner-supplied original spawn, zone 490 / instance 0, original character and completed personal task 920167 or 920168. It rejects the shared 920169 → 920170 edge and nested private dispatch before taking a handoff. It consumes the existing noncopyable R2 handoff once, verifies its original finish identity, then calls the existing three-argument `Client::AssignTask` once.

A scoped thread-local pointer refers only to this synchronous stack frame. The direct matching `AcceptNewTask` invocation claims it once at entry. Reentrant acceptances cannot claim it again. It is neither a character/task registry nor retry or recovery state. Its scope destructor only restores the previous pointer.

Inside the existing successful personal reward reservation block, the bridge retains the actual `ReserveTaskRewardInstance` result and its accepted time. At the existing save position, it retains the return from that same `SaveClientState` invocation. Before mutable `TASK_ACCEPTED` callbacks, the reviewed `BindAccepted` checks the actual slot and fresh persisted occurrence/task/four initial activity rows, using those exact results. Only a successful binding records acceptance against the same one-taken R2 handoff. Neither helper allocates or saves again.

After `AssignTask` returns, the dispatcher reads only its stack result. It does not dereference Client, task state, active slot, NPC or handoff after callbacks. `AcceptedBeforeCallbacks` means the actual fresh persisted acceptance was recorded before callbacks; callbacks may subsequently change the task. Legacy result `SubmittedNotAccepted` means acceptance is unconfirmed, and does not establish that no task exists. It must never trigger cancellation, a new allocation, automatic retry or historical reconstruction.

## Preserved behavior and integration boundary

Unrelated task calls have no private frame and incur no new persisted-state query. The native save still executes once at its original position, and native callbacks retain their existing behavior, including when the save returns false. Early rejection, missing reservation, save failure, unknown/mismatched persisted rows and absent task state yield no acceptance receipt. No public getter, scripting API, new dispatch parameter, schema, CMake target or global retry registry is added.

The occurrence and R2 namespace bodies are unchanged from the hash-pinned Spells inputs; only their native include/license prologues are supplied. The private dispatch declaration and implementation replace the reviewed disabled dispatcher's unobservable void submission with this actual acceptance-frame bridge.

This patch does not activate the entire Fire feature. The original physical finish owner must retain the authentic original occurrence, successful reward, exact ledger and actual cleanup/completion-save observations, and supply the one original R2 handoff and real selected spawn. Initial 920167 acceptance binding and correlated World admission for 920170 remain their existing owner seams. No spawn identity, reward/debit proof, cleanup success, shared admission or recovery policy is invented here.

## Integration and rollback boundary

Use `native-integration.patch` only with the exact listed relative preimages and absent-header conditions. Use `native-rollback.patch` only with all four exact listed postimages; it restores the original task source and removes the three newly added headers. A changed preimage or postimage requires reconciliation. Publication makes neither operation authorized against a live checkout.

The owner retained a revision 1 isolated preimage applicability check; that check was not rerun for the one-line revision 2 optional dereference correction. This publication verified exact artifact/postimage hashes and the published branch mismatch. No patch application, compilation, tests, database reads or writes, deployment or runtime action was performed for publication. Build and native behavior remain unverified. Initial personal acceptance, original cleanup/reward observations, physical-finish caller wiring, shared World admission and recovery policy remain separate owner seams.
