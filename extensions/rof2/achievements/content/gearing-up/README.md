# Gearing Up I–VIII

Eight repeatable quests from Tabazin Gearflinger in Fortress Mechanotus, with all 15 item-delivery objectives and eight component bindings for achievement **500120600**. Hail Tabazin and choose **brave enough** to open the native task selector. One Gearing Up task may be active at a time.

**Status:** enabled content installed locally; world and six zone caches refreshed. Combined in-game item/coin custody, completion, reward and persistence acceptance is still pending. This supersedes the disabled r1/r2 package.

## What is included

All four required Lua files, guarded SQL, provenance, configuration record and installer are included. Existing NPC placement and 13 item drop routes are retained. No housing data, player data, client assets or test scripts are included.

Minimum level **75**, base faction **+10** for Ak`Anon Strike Force V (1190), and **1000 XP** are labelled server defaults. Original minimum/reward amounts remain unverified. Native modifiers can change actual rewards. Cash reward is zero; no extra parent item reward or faction admission threshold is invented. `settings.json` records these values and their provenance; the installer uses the frozen SQL/Lua, and does not dynamically regenerate them. Change minimum level in both the policy and task definitions; change XP/faction in task definitions using a reviewed migration, then regenerate package hashes.

## Required server support

Use the achievements/shared-core engine with native delivery custody commit `94e5f72768635a504c9b13feed093ddcef81d82b` or its reviewed descendant, built and deployed. The Lua handler must exclusively own this NPC's trade callback. It reconciles native task deliveries once and returns unused items/coins once. Unknown existing callbacks are refused. The standalone adapter does not install Oculus; an existing Oculus adapter needs explicit reviewed composition.

## Install on Linux, akk-stack or native Windows

1. Back up the database and quests. Stop the affected server. Confirm the required engine is actually deployed, along with the existing achievements catalogue, NPC 436006, faction 1190 and item definitions.
2. Install Python 3 and a MariaDB command-line client. For akk-stack, the command can invoke the database container. Native Windows can use `mariadb.exe`; this is content installation, not a claim of MSVC engine-build acceptance.
3. Outside the repository, create a JSON file containing the database client's argument array. Use an external credentials file or container environment; keep passwords out of the repository. Include the target database and batch mode. SQL is passed through standard input; shell command strings are not accepted.
4. Inspect first, then apply using a **new, nonexistent backup directory**:

```text
python install.py --quests-dir PATH_TO_QUESTS --backup-dir NEW_BACKUP_PATH --db-command-file PRIVATE_ARGV_JSON
python install.py --quests-dir PATH_TO_QUESTS --backup-dir NEW_BACKUP_PATH --db-command-file PRIVATE_ARGV_JSON --engine-reviewed --server-stopped --apply
```

Example local client argument array (adjust paths and database):

```json
["mariadb", "--defaults-extra-file=/private/db.cnf", "--batch", "peq"]
```

For Windows, use `python` or `py -3`, escaped Windows paths in JSON, and the full `mariadb.exe` path if needed. For akk-stack, use an argv array beginning `docker`, `exec`, `-i`, followed by your database container and its client command. Choose the actual mounted quests directory on the host.

5. Inspect the private installation receipt. Start the server with fresh task/achievement caches and quest VMs. Verify native selection, deliveries, returned surplus/coins, rewards, eight components and one ordinary persistence cycle.

The installer checks package hashes and reviewed file preimages. SQL transactionally inserts exactly eight tasks, 15 activities and eight criteria; an exact existing payload is a no-op and drift/collisions fail. It never changes the global database version. Reservation **9340** must be registered after shared migration **9339** by the source owner; do not skip pending migrations. If installation fails, stop and inspect the receipt: SQL and filesystem updates are separate phases, with backups and no automatic replay or guessed rollback. Restoring scripts alone does not undo committed SQL or earned player progress.

## Opcodes and structures

This content uses the repository's existing RoF2 native task and achievement protocol implementation. **Recovered opcodes and packet structures remain available in the achievements branch's protocol documentation and engine source.** This quest batch introduces no new opcode or structure and does not imply every achievement protocol gap is resolved.

See `PROVENANCE.json`, `APPLIED-STATUS.json` and `PAYLOAD-MANIFEST.json` for research, validation boundaries and exact payload hashes.
