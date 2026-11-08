# RoF2 server runtime — @PLATFORM@

Built from commit `@COMMIT@`. This package contains platform-native server binaries and helpers, matching quest/opcode content, a full sanitized database export, and 331 portable Housing JSON files. Read `CONTENT-LIMITS.md` and `database/README.md` before installation. Packaging/build success does not establish fresh native gameplay acceptance on this platform.

## Requirements

Use Python 3.9 or newer, the MariaDB client on PATH, and a **new empty latin1 MariaDB 10.11 database**. The bundled full dump replaces tables: never import it over a server containing player data. Accounts, characters and private operational data are not supplied. Configure your own login accounts/server registration after installation using normal administration tools.

The exact build ABI and runtime dependency inventory follows. Install matching libraries; the ZIP is not a substitute for its operating system or Perl distribution. Linux is built on the recorded Actions runner distribution, not an asserted older distribution baseline. Windows requires x64 Windows, a compatible Microsoft VC++ x64 runtime, the full matching Strawberry Perl x64 distribution on PATH, and Python's `py -3` launcher; vendor DLLs listed as bundled accompany the executables. Do not copy Linux binaries into Windows.

```json
@REQUIREMENTS@
```

## Fresh installation

1. Verify the downloaded ZIP against its `.zip.sha256` or the release's `SHA256SUMS`, then extract it. The package's root `SHA256SUMS` covers the installed file layout. `database/SHA256SUMS` is retained unchanged as provenance of the original database input layout; its `housing-runtime-content/` entries are relocated to `server/` in this package.
2. Create the new empty latin1 database and a private operator account. Import `database/full.sql` with your MariaDB client's normal credential handling. No installer connects to a database automatically. Carry the database README, table inventory and completed-recipe qualifications with this installation.
3. In `server/`, copy `eqemu_config.json.example` to `eqemu_config.json` and `login.json.example` to `login.json`. Replace every `REPLACE_...` value, database names, addresses and world key. Keep these private files out of Git and release archives. Restrict access to this directory to the operator account (on Windows use a private user directory/ACL).
4. Supply licensed original RoF2 client resources and supported server maps yourself. Proprietary client archives, zone geometry and maps are not distributed. From the package root run `python tools/housing/install_original_geometry.py --client "PATH_TO_ORIGINAL_ROF2" --server server --overlay tools/housing/catalog` (use `python3` on Linux). The importer reads the client and verifies the same-source catalog; do not substitute unrelated resources. Install your authorized server maps under `server/maps/`.
5. From `server/`, run `bin/shared_memory` on Linux or `bin\shared_memory.exe` on Windows to generate this fresh target's shared data. It uses your configured database. Use the same working directory for every server executable.
6. Set `EQEMU_DATABASE_NAME` to the configured database name if it differs from `peq`; optionally set `EQEMU_ZONE_COUNT` (default 6). Start `@LAUNCH@` from `server/`. Linux uses Python 3; Windows creates a dedicated interactive console and uses `py -3`. Keep Windows in that console; service/headless launch is not provided here. Use Ctrl+C for normal shutdown and allow owned children to finish. The supplied inventory supervisor checks its optional schema and certifies only children it started and reaped; a crash does not authorize takeover or recovery of unknown processes. Journal files are flushed on both platforms; directory entries are flushed only on POSIX, so Windows directory-entry crash durability is not guaranteed. `eqlaunch` is included for its standard uses but does not replace supervised inventory ownership.

The helper `inventory_recovery_cli` and `export_client_files` are built from this exact server commit. The latter is an operator tool; CI does not connect to a serving database or export private data. Runtime logs, generated shared memory, player state, credentials and exported client files belong only to your installation.

Preserve LOCAL/reconstruction/source-unconfirmed and unfinished-content qualifications. The three open recipe bounds do not gain an invented scalar, and inactive/unfinished features are not made accepted by packaging. For upgrades to an existing player database, obtain a separately reviewed additive migration plan; this full database dump is a fresh-install input only.

## Mercenary level 86–100 qualification

Read [the level-100 content guide](common/mercenary/LEVEL100.md). Its stat curves and NPC repertoire are documented emulator reconstruction; immutable May 2013 spell definitions, exact historical Live balance and historical NPC casts are not authenticated. The matched sanitized database supplies the reviewed content; build/package success does not establish native gameplay acceptance. Existing player databases need the separately coordinated additive migration and normal mercenary catalogue cache refresh. Do not import this package's full database over player data.
