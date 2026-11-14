# EverForge RoF2 0.3.0

EverForge runs the published Rain of Fear 2 server and matching custom database, with Spire administration, PEQ Editor, phpMyAdmin, SSH/SFTP, explicit FTPS and local scheduled backups. It is a community project, with no official EQEmu or Daybreak affiliation.

Spire and PEQ Editor are built from our maintained forks. [utilities-sources.json](everforge/utilities-sources.json) identifies the exact source commits and archive checksums. Normal authentication and Spire permissions remain active. Authenticated operators can administer the whole game schema; the previous content-table grants and deny-default gateway are removed.

## Install

Use Linux amd64, Docker with Compose v2 and Python 3.11+ on the host. Allow at least 20 GB for the server, source-build caches and backups. First installation needs Internet access. Utility installation builds Spire's frontend and Go binary and the PEQ PHP image inside Docker; host compilers are unnecessary. The native runtime uses Ubuntu 26.04 and the published glibc 2.43 / Perl 5.40 ABI.

Extract the setup files:

```sh
mkdir everforge
docker run --rm --entrypoint tar nightsta69/everforge-rof2:0.3.0 \
  -C /opt/everforge/setup -cf - . | tar -xf - -C everforge
cd everforge
python3 configure.py
```

Edit the private `.env`. Set `PUBLIC_ADDRESS` to the address game clients use, `LOCAL_ADDRESS` to the LAN address, and `BIND_ADDRESS` to the host interface that should serve the game. Set `UTILITY_BIND_ADDRESS` for administrative access and `FTP_PUBLIC_HOST` for passive FTPS. Defaults bind published ports to loopback. Place your SSH public key in `authorized_keys`, or set `SSH_AUTHORIZED_KEYS_FILE` to its existing path. Password-only SSH needs an existing empty key file and an explicit `SSH_PASSWORD`.

Generated passwords remain in `.env`, mode 600. No shared public password is supplied. [PORTS.md](everforge/PORTS.md) lists every protocol, bind setting and firewall/router forwarding requirement. Setup does not change the host firewall, host SSH daemon or router.

### Provide the required original resources

The published native server requires 13 exact housing EQGs and 13 matching maps. Supply your authorized originals as read-only external mounts. They are not included in the image, source bundle or backups. Public maps and optional mesh sidecars do not replace these witnesses.

```sh
mkdir -p ../everforge-data
# This directory must be writable by container UID/GID 1000.
docker run --rm -v "$(realpath ../everforge-data):/data" \
  nightsta69/everforge-rof2:0.3.0 verify-release
```

Use the tag printed by that command:

```sh
python3 provision-resources.py \
  --release-directory ../everforge-data/releases/TAG_FROM_LOG/payload \
  --archive-directory /absolute/operator/original-eqgs \
  --map-directory /absolute/operator/matching-housing-maps \
  --server-data-directory ../everforge-data \
  --output-directory ../everforge-operator
```

Optionally add `--original-mesh-directory /absolute/operator/housing-original-meshes`. The helper verifies the downloaded release's inventory and every required resource hash before generating its override. Missing resources stop startup before DB access and produce `/data/missing-resources.json`.

### Build and lock the utilities

```sh
python3 utilities-install.py install \
  --directory ../everforge-utilities \
  --server-data-directory ../everforge-data
```

This builds the two reviewed forks, installs supported SSH/FTP packages, pulls phpMyAdmin, and builds the matching MariaDB backup image. It verifies source/asset checksums and records commits, binary hashes, image IDs and package versions in `install-lock.json`. An existing installation is reused by `install`; normal restarts do not reinstall or upgrade software. Optional `--download-cache /path` reuses verified public downloads.

Set `COMPOSE_FILE` in `.env`, preserving this order and using actual absolute override paths:

```dotenv
COMPOSE_FILE=compose.yaml:compose.utilities.yaml:/absolute/everforge-operator/operator-resources.override.yaml:/absolute/everforge-utilities/utilities.override.yaml
```

If you use a private environment file other than the setup directory's `.env`, also set `EVERFORGE_ENV_FILE` to that file for backup coverage.

### Initialize, then start

```sh
docker compose up -d mariadb
docker compose run --rm server initialize
docker compose run --rm utility-init
docker compose --profile utilities up -d
```

`initialize` validates resources, seeds only an empty database and installs persistent server files without launching native processes. `utility-init` provisions authenticated utility identities and the dedicated World console account. Running Spire receives the game account, not the MariaDB root credential. The complete utility profile starts the integrated Spire server plus SSH, FTP, phpMyAdmin, PEQ Editor and backups. Optional sidecar profiles are `ssh`, `ftp`, `admin`, `editor` and `backups`.

For the game alone, omit `compose.utilities.yaml` and the utility override, then use the core Compose file plus the operator-resource override. Spire is integrated with the native server when the utility configuration is included.

## Access and controls

| Utility | Default address | Authentication |
| --- | --- | --- |
| Spire | `http://HOST:3000` | `SPIRE_ADMIN_USER` / `SPIRE_ADMIN_PASSWORD` |
| phpMyAdmin | `http://HOST:8080` | `everforge_editor` / `EDITOR_DB_PASSWORD`; DB host `mariadb` |
| PEQ Editor | `http://HOST:8081` | Native PEQ login: `PEQ_EDITOR_USER` / `PEQ_EDITOR_PASSWORD` on first initialization |
| SSH / SFTP | `HOST:2222` | User `eqemu`, authorized key or explicitly enabled SSH password |
| Explicit FTPS | `HOST:2121` | User `ftpuser` / `FTP_PASSWORD`; passive TCP 30000–30009 |

PEQ uses its own administrator/user management. Existing PEQ accounts and passwords are preserved; environment credentials create an administrator only when the user table is empty. The upstream `admin/password` seed is never imported.

Spire's dashboard and counts come from the real World console and native processes. Its start/stop/restart and delayed cancellation controls use the container's process manager. Stopping the game keeps Spire available. The manager retains the published direct-parent inventory supervisor's Zone birth, reaping and recovery logic. Ordinary child failures recover through that supervisor; an inventory recovery failure is reported for operator review. It does not fabricate ownership after a supervisor crash.

Process CPU percentages are averages since process start; process memory is resident usage relative to system memory. System widgets use native Spire measurements.

Native process options and dynamic Zone count are applied on a deliberate start/restart. Zone capacity follows the configured game port range; adjust Compose mappings and network rules when changing that range. The package's default is UDP 7000–7010. Native Zone log websocket traffic travels through the Spire origin, so internal TCP listeners need no additional public forwarding.

SSH starts in `/data/server`, with server libraries and the server's PID/network namespace. SFTP and FTPS transfer directly into the persistent serving tree. FTPS requires TLS for both control and data by default. Its certificate persists in `ftp-state`; export `ftp:/state/ftps.crt` and verify/trust it in your client. `FTP_TLS_REQUIRED=false` explicitly permits plaintext compatibility. FTP is chrooted to the server directory. SSH host keys persist in `ssh-state`.

The CLI offers `start`, `stop`, `restart`, `logs`, `shell`, `backup`, `backup-list` and `restore`:

```sh
python3 everforge.py shell
python3 everforge.py logs
```

CLI stop/restart operates the server container; Spire's controls operate the native game processes while keeping administration available.

## Persistent files and updates

`/data/server` is the writable installed server: configuration, quests, binaries and exports. Operator edits survive restarts and recreation. `/data/releases` holds the checksummed download cache; startup validates it and does not copy it over an existing serving tree. Public maps are installed into the serving tree; required original witnesses remain external read-only links. Legacy quest overlays are copied into the serving tree on initial upgrade, after which direct edits belong in `/data/server/quests`.

Spire configuration saves preserve custom native fields, including `server.auto_database_updates=false`. Startup never automatically installs an upstream server, imports a new full SQL dump into an existing database, or resets custom content. Existing database-local bootstrap markers and schema/resource compatibility are checked before launch.

Spire's release page lists this project's published server assets. Stop the native server before installing selected binaries. The adapter verifies the release/ABI/resource/database contract, retains previous binaries, and replaces only the installed binary directory. Configuration, quests and DB data remain in place. A persisted selection survives container restart. `EQEMU_RELEASE` selects the initial installation; ordinary startup retains the installed selection instead of relabeling unchanged binaries as a newly downloaded release. A release needing a different database contract requires a reviewed database upgrade; no full dump is applied automatically.

For an explicit binary update without Spire, stop the server container and use:

```sh
docker compose stop server
docker compose run --rm server install-release PUBLISHED_TAG
docker compose --profile utilities up -d server ssh
```

The bootstrap lock refuses a concurrent update against a running server container. The CLI uses the same verified project release installation path.

Source-build tools are available for an operator-supplied checkout and configured build directory. Upstream installation/import actions remain explicit operator operations on writable paths. They are not run at startup. A stock upstream installation can replace custom project components; use the published-project release path for this restoration pack.

Utility updates use our reviewed fork pins rather than replacing custom additions with upstream releases. Obtain the newer EverForge setup, preserve `.env` and data, then run:

```sh
python3 utilities-install.py update --directory ../everforge-utilities \
  --server-data-directory ../everforge-data
docker compose --profile utilities up -d
```

New builds finish before the lock and override switch. Prior utility payloads and manifests remain available. This does not reseed the game DB.

## Upgrade from 0.1 or 0.2

Stop the old stack using its existing setup, keeping all volumes and private configuration. Extract the new setup into that same setup directory without deleting `.env` or operator overrides. Run `python3 configure.py --add-utilities`, set `BOOTSTRAP_IMAGE_TAG=0.3.0`, and use `utilities-install.py update` above. Keep the same Compose project name, database volume and server data directory.

Run the initialization/start sequence above. Existing server config and legacy quest edits are copied once into `/data/server`. Initialized standalone Spire metadata is copied into the native game's empty `spire_*` tables, preserving the original encryption/JWT keys and users. A private pre-migration SQL snapshot and receipt are retained under `/data/admin`; the old schema/volume are kept. Conflicting initialized metadata stops for reconciliation instead of overwriting either copy. Old standalone backend/gateway containers can remain stopped; no volume pruning is required.

## Backup and restore

```sh
python3 everforge.py backup
python3 everforge.py backup-list
```

The scheduler uses five numeric cron fields, `BACKUP_SCHEDULE` (default `0 3 * * *`), `TZ` and `BACKUP_RETENTION_DAYS` (default 14). The matching MariaDB 10.11 dump includes both schemas, routines, events and triggers using one InnoDB transaction. File coverage includes the mutable server, direct quest edits, configuration, Spire keys/metadata state, administration control state, SSH host keys, FTPS certificates and the private operator environment. Release caches, generated shared memory/logs, maps, required external original resources and known client archive formats are excluded. Keep file editing quiescent for a consistent file snapshot alongside the DB transaction. Backups contain private credentials and player data; retain them privately.

For a clean inventory recovery checkpoint, stop the native game through Spire, create the complete archive, then start it again. This lets the owning supervisor reap its children and finish inventory recovery before the snapshot. Live scheduled copies retain any unreaped ownership rows; recovery may require review before affected actors can enter the recovered game. The helper never marks old source processes dead merely because a snapshot was restored.

Spire's native SQL backup/download facility also remains available. The complete recovery archive is produced by `everforge.py backup` or the scheduler.

Restore into a separate empty project and directory:

```sh
python3 everforge.py restore \
  --utility-install ../everforge-utilities \
  --target-project everforge-recovery \
  --target-directory ../everforge-recovery \
  --archive everforge-TIMESTAMP.tar.gz
```

The helper verifies archive/SQL hashes and refuses nonempty target schemas or files. It creates a separate DB with no host-published DB port and retains the source. Restored directories include `server-data`, `operator-data`, `spire-state`, `ssh-state`, `ftp-state` and `operator-config/.env`. Keep the target's `.restore-root-password`; set the recovered environment's root password to that value, retain its original game/utility credentials, and point recovery Compose mounts at the restored directories. Reattach the required original resource mounts and the reviewed utility payload; regenerate public map/cache files from the release if needed. Run `utility-init` against the target DB before serving it. Original MariaDB system users/grants are not replayed.

## Validation and scope

See [VERIFICATION.md](everforge/VERIFICATION.md) and [TEST-RECEIPTS.json](everforge/TEST-RECEIPTS.json) for the tested image, fork identities and actual utility/restore checks. PEQ compatibility covers the reviewed schema 9338 additions and retains warnings for newer unsupported schemas. Representative editor writes are verified; this does not claim every editor page, destructive player/custody operation or native gameplay system has been exhaustively accepted. Map format inventory and native loader compatibility are recorded in [MAP-FORMATS.json](everforge/MAP-FORMATS.json).
