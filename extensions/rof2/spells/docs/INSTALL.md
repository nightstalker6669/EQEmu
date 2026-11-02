# Install spell content

Use a supported current PEQ/MariaDB schema with the original spell/item/NPC content corresponding to this overlay. This is a targeted content repair, not a complete PEQ database or a replacement `spells_new` import. IDs are deliberately fixed and collision checks refuse customized rows rather than silently remapping them.

The [pinned shared-core reward engine](../README.md#engine-ownership-and-dependency) is already integrated into this branch. Build this branch's matching world/zone binaries; no achievement branch, manager or quest content is required. Run the matching database updater through 9336 and review/apply the three original supplemental SQL files described below before installing spell content. Preserve their identities; do not create renamed duplicates.

- `utils/sql/git/2026_10_27_achievement_reward_classes.sql`: generic option class masks; its historical filename is retained.
- `utils/sql/git/2026_10_30_shared_task_reward_identity.sql`: durable shared occurrence ownership in the player database.
- `utils/sql/git/2026_10_30_shared_task_admission_policies.sql`: optional admission policy in the content database.

Follow the [shared reward schema compatibility guide](../../../../docs/rof2/SHARED-REWARDS.md), preserving state/content routing and reviewing existing deployments for matching schema before applying additive SQL. Updater 9331/9332 retains empty achievement table scaffolding as a compatibility envelope, not achievement gameplay. Apply no second copy of those migrations.

Back up the database and quests, stop world and all zone writers through the normal service manager, and coordinate any shared test/runtime work. This installer never stops, starts, reloads or rebuilds services, replaces shared memory, or touches accounts/characters/inventories. It is intended for offline content installation.

## Alaran merchant requirements

Migration 9336 adds `merchantlist.language_required` (signed SMALLINT, default -1) and `language_skill_required` (unsigned TINYINT, default 0). Default rows remain unrestricted. Language 25 is Alaran. Stage 005 sets the sourced 25/50 skill requirements on Argath rank I stock. Both list and buy paths enforce the same requirement, including normal, alternate-currency and adventure merchants. Invalid language IDs or thresholds fail closed before indexing the character profile.

Run the matching updater against the content database before starting the new binaries. The registered migration and `utils/sql/git/2026_11_01_merchant_language_requirements.sql` are the same upgrade: do not invent another migration ID. Its schema guard rejects incompatible existing columns. The content installer also refuses a missing schema or changed vendor identity. Old binaries ignore these new fields and would bypass the restriction, so binaries and schema must be deployed together. Merchant catalogues are zone-cached and need the normal coordinated activation.

Stage 005 does not implement the separate Cantrip/Periapt turn-ins, change language learning, or infer thresholds for conflicting guide entries. Its rollback reverses content only. Keep the additive schema while any matching binary uses it.

## Python environment and database connection

Use Python 3.9 or later and PyMySQL in a private virtual environment. Keep connection configuration outside the checkout and restrict its permissions. The JSON object accepts `host`, `port`, `user`, `password`, `database` and optionally `unix_socket`. Supply the credentials normally used for administering the intended server database. No connection settings are bundled in this branch.

From the repository root, plan first:

```sh
python3 extensions/rof2/spells/tools/install.py \
  --db-config /private/spells-db.json \
  --server /srv/eqemu/server
```

The plan selects stages 001, 002, 004 and 005. Add `--include-estimates` only when deliberately selecting the labelled pet/aura reconstruction too. The planner checks table/column availability, InnoDB transactions, row collisions, vendor identities/bindings, external item/NPC/AI/spell references, required task/hand-in rules, and the expected rank/class level of every Kerkaz reward. It does not change persistent content or quest files. Matching already-installed rows are a no-op; conflicting rows abort the plan.

After reviewing the plan, apply with a new private backup directory:

```sh
python3 extensions/rof2/spells/tools/install.py \
  --db-config /private/spells-db.json \
  --server /srv/eqemu/server \
  --backup /srv/backups/rof2-spells-01 \
  --apply
```

Use the same estimate flag for plan and application. The backup records exactly the newly created rows, prior vendor field values, and the quest-file preimage. The installer refuses to overwrite a different Kerkaz script. Merge any existing script deliberately before installation; other Shard's Landing scripts and global handlers are not replaced.

Native Windows: use `py -3` and native absolute paths. Akk-stack: run the installer in an appropriate one-off administrative Python environment with the checkout and server quest mounts, and database connectivity. Native Windows and Akk-stack execution of this installer have not been tested; the Python/MariaDB implementation was exercised on Linux.

## Activation and acceptance

Regenerate the normal item/spell shared-memory cohort where required by the server installation, including the five newly restored items. Deploy matching binaries/schema before starting services. Use the normal supported quest/task reload only if a coordinated live-content upgrade is chosen instead of the offline workflow. Idle zone processes also cache task definitions; reloading only one occupied zone does not update every process.

Kerkaz intentionally has **no `event_trade`**. In the pinned engine, native `FinishTrade` first credits delivery, then the no-script trade branch removes the exact credited quantity with `RemoveTaskDeliveredItems`. `Items:AlwaysReturnHandins=true` returns wrong items and unused stack quantities. A generic scripted `return_items` callback would bypass that consumption branch and could return a mote already credited to a task. Keep the existing task engine authoritative; do not add manual reward summoning or currency conversion.

With an independently assigned test character, verify both selectors, the five correctly named tasks per selector, one-mote consumption from a stack, wrong-item return, class filtering, documented multi-scroll bundles, single-claim delivery and repeatability. Test Glorious Expurgation III and Daring Escape III after their item records enter the cache. These native acceptance cases remain pending for this snapshot.

## Rollback

Keep the private installation backup. The scripts under `sql/rollback-*` reverse the original full content additions; they are not an automatic rollback for a partial/no-op installation. Restore only the rows/fields actually changed by your receipt, in reverse dependency order. Do not remove independently added or subsequently edited content.

Before removing task reward definitions or items from a used server, account for pending native reward choices and owned/active content. This package does not delete character task history or reward ledgers. Remove the new Kerkaz script only if its hash still matches the installed file; otherwise merge the rollback with later changes. Do not reset inventory leases, clear character inventories or replace a whole database to undo this content overlay.
