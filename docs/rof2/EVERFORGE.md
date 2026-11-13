[Back to the restoration pack](../../README.md) · [EverForge on Docker Hub](https://hub.docker.com/r/nightsta69/everforge-rof2)

This guide describes the tested **0.2.0** setup. The server release downloaded at startup is versioned separately from the Docker runtime.

# EverForge RoF2 — EQEmu Restoration Pack 0.2.0

A Rain of Fear 2 server stack with restored systems, release-based updates and optional operator utilities. Ready to run after the required operator resource setup. Community project; no official EQEmu or Daybreak affiliation or claim that every feature is complete.

The core runtime downloads the latest stable published `nightstalker6669/EQEmu` Linux release on startup, follows that release's matching public database contract, verifies hashes/ABI and preserves existing database volumes. Optional utilities provide Spire content editing, SSH/SFTP, FTPS, phpMyAdmin, PEQ Editor and scheduled local backups.

## Requirements and setup files

Linux amd64, Docker with Compose v2, Python 3.11+ on the operator host, Internet access for first setup and sufficient persistent storage (allow 15 GB or more including backups). Compilers are not required on the host. The core runtime is Ubuntu 26.04 with the published glibc 2.43 / Perl 5.40 ABI.

Extract the public setup files from the image, or unpack the supplied setup archive:

```sh
mkdir everforge
docker run --rm --entrypoint tar nightsta69/everforge-rof2:0.2.0 \
  -C /opt/everforge/setup -cf - . | tar -xf - -C everforge
cd everforge
python3 configure.py
```

Edit the private `.env` file. Set `PUBLIC_ADDRESS` to the IP/DNS name clients use; set `LOCAL_ADDRESS` and `BIND_ADDRESS` to the host's LAN address for LAN use. Utility ports bind to loopback unless `UTILITY_BIND_ADDRESS` is changed. Set `FTP_PUBLIC_HOST` to the address FTP clients can reach. Put your SSH public key in `authorized_keys`, or set `SSH_AUTHORIZED_KEYS_FILE` to an existing public-key file. No default public password is supplied; generated secrets stay in `.env` mode 600.

**Upgrade from 0.1.0:** preserve the existing `.env`, database and data directories. Run `python3 configure.py --add-utilities`, set `BOOTSTRAP_IMAGE_TAG=0.2.0`, then complete utility setup below. Do not replace existing database credentials or delete volumes.

## Required original resources

The unchanged published release requires 13 exact EQG archive witnesses and 13 matching housing maps during Zone startup. Public maps alone are insufficient. These operator-owned inputs are never baked into the image, copied into releases or included in publication bundles.

First fetch/verify the current release without starting a database:

```sh
mkdir -p ../everforge-data
# Bind directory must be writable by runtime UID/GID 1000.
docker run --rm -v "$(realpath ../everforge-data):/data" \
  nightsta69/everforge-rof2:0.2.0 verify-release
```

The log prints the selected tag. Replace `TAG_FROM_LOG` and the resource paths below with your actual values:

```sh
python3 provision-resources.py \
  --release-directory ../everforge-data/releases/TAG_FROM_LOG/payload \
  --archive-directory /absolute/operator/original-eqgs \
  --map-directory /absolute/operator/matching-housing-maps \
  --server-data-directory ../everforge-data \
  --output-directory ../everforge-operator
```

Optionally add `--original-mesh-directory /absolute/operator/housing-original-meshes`. The published mesh installer produces additional sidecars; it does **not** produce the required full EQGs/maps. No map generator is invented here. Obtain exact matching maps with documented provenance from your authorized installation or compatible tools. `REQUIRED-HOUSING-WITNESSES.json` records the current published selection requirements; the helper always reads the downloaded release's own manifests and verifies every resource before emitting read-only mounts.

Missing or changed resources produce `/data/missing-resources.json` and stop before DB access. Resource mounts stay outside versioned downloads; a later release revalidates its own requirements. Keep the generated override/receipt private and outside this source directory.

## Install utilities and lock versions

```sh
python3 utilities-install.py install \
  --directory ../everforge-utilities \
  --server-data-directory ../everforge-data
```

First install resolves the current stable compatible official Spire release, its preview assets, current phpMyAdmin image and maintained PEQ Editor source. It pulls supported OS/PHP/MariaDB base images and builds the SSH/FTP, backup and PEQ sidecars once using current supported packages. This is a setup-time build; there is no apt upgrade, software reinstall or version check on ordinary service restart.

The persistent `install-lock.json` records selected versions, digests, source revision and package versions. Versioned utility payloads and generated `utilities.override.yaml` retain those selections. Re-running `install` with an existing lock reuses it, including without network access. Optional `--download-cache /path` reuses previously verified public downloads.

The tested compatible Spire API set is v4.24.2. A future unreviewed release fails clearly and preserves the installation; it is not silently treated as compatible. PEQ source has no release series here, so its maintained master commit is resolved once and recorded. Images/base versions are not described as “latest forever.”

Add the following to `.env`, using actual absolute override paths and preserving this order:

```dotenv
COMPOSE_FILE=compose.yaml:compose.utilities.yaml:/absolute/everforge-operator/operator-resources.override.yaml:/absolute/everforge-utilities/utilities.override.yaml
```

Start the core, provision dedicated utility identities, then start utilities:

```sh
docker compose up -d mariadb server
# Wait for the server's successful startup in the logs before provisioning editors.
docker compose logs -f server
docker compose run --rm utility-init
docker compose --profile utilities up -d
```

Individual profiles: `spire`, `ssh`, `ftp`, `admin`, `editor`, `backups`. `utilities` enables all six utilities. `utility-init` alone receives provisioning authority; running Spire does not receive root/runtime database credentials.

For an intentional utility update, stop utility writers and back up first, then:

```sh
python3 utilities-install.py update --directory ../everforge-utilities \
  --server-data-directory ../everforge-data
docker compose --profile utilities up -d --force-recreate
```

New downloads/builds complete before the lock/override changes. Previously selected payloads remain on disk; existing running containers keep their selected payload mount until explicitly recreated. This utility update does not update or reinstall the custom EQEmu server/database.

## Services and ports

| Service | Host ports / protocol | Login / scope |
| --- | --- | --- |
| RoF2/SoD login | 5999 UDP | Authorized game client |
| Legacy login listener | 5998 UDP | Only for that protocol |
| World | 9000 UDP | Game clients |
| Zones | 7000–7010 UDP | Matches runtime port range |
| Universal chat/mail | 7778 UDP | Game clients |
| Spire | 3000 TCP | `SPIRE_ADMIN_USER` / `SPIRE_ADMIN_PASSWORD` |
| SSH and SFTP | 2222 TCP | User `eqemu`, authorized public key by default |
| Explicit FTPS | 2121 TCP + 30000–30009 TCP | User `ftpuser` / `FTP_PASSWORD` |
| phpMyAdmin | 8080 TCP | User `everforge_editor` / `EDITOR_DB_PASSWORD`, server `mariadb` |
| PEQ Editor | 8081 TCP | `PEQ_EDITOR_USER` / `PEQ_EDITOR_PASSWORD` |
| Scheduled backups | None | Local volume / CLI |

MariaDB 3306 TCP, world interserver 9001 TCP, login registration 5998 TCP and Spire's backend 3001 are not published. World HTTP/telnet remain disabled. These are container services; installation does not modify host sshd, host firewall or router rules.

For LAN utility access set `UTILITY_BIND_ADDRESS` to the host LAN IP and allow the selected TCP ports from your administrative LAN. Internet game access requires forwarding the game UDP ports to the host's LAN IP. Ordinary game access does not require forwarding database/admin ports. Docker publishing and any host firewall/router rules must agree.

Spire is `http://HOST:3000`, phpMyAdmin `http://HOST:8080`, PEQ Editor `http://HOST:8081`. Use trusted LAN access or SSH forwarding for HTTP administration. SSH password authentication is off unless explicitly set; a password-only setup still needs an existing empty `authorized_keys` file for the configured bind. Server host keys persist in `ssh-state`.

FTPS requires TLS for login and data by default. The first start creates a persistent self-signed certificate in `ftp-state`; export its public certificate with `docker compose cp ftp:/state/ftps.crt ./ftps.crt` and verify/trust it in the client. Port 2121 uses explicit FTP-over-TLS, not implicit TLS/SFTP. `FTP_TLS_REQUIRED=false` is an explicit plaintext compatibility choice. Passive addressing uses `FTP_PUBLIC_HOST` and fixed 30000–30009 ports. The supported Pure-FTPd package requires `SYS_NICE` and `DAC_READ_SEARCH` in its container; it is not privileged and receives no host or server-cache mount.

## Protected Spire and operator files

Spire is a **content editor** in this package. A standalone working directory and explicit separate DSNs keep its metadata in the `spire` schema. The game editor can SELECT the game schema and perform DML only on the 68 reviewed existing content tables in `utilities/content-scope.json`; it cannot perform game DDL, change schema version/housing activation or write player/login/custody tables. Empty pre-existing `spire_*` tables in the public game seed remain unchanged and unused.

A deny-by-default HTTP gateway permits the reviewed content CRUD routes and limited login/read operations. It rejects installer, binary/content updater, import/raw-SQL, configuration replacement, build/process control, Spire updater and onboarding reset requests before forwarding. The backend has no published port, resides on an isolated internal network and receives no server binaries/quests/opcodes/resources/configuration or Docker socket. Actual prohibited install/update attempts were tested with unchanged full game data/schema/payload fingerprints. Limited server-management screens may show denied operations; this is intentional and enforced at the backend gateway, not by hiding buttons.

Spire's own native backup button is unavailable because it delegates to the server and does not provide this package's backup contract. Use the backup CLI below. The separate metadata schema may run Spire's own metadata migrations; it cannot run game migrations. Spire preview assets are verified/locked at setup, mounted read-only and use upstream's supported `SKIP_ASSET_CHECK` to prevent automatic asset replacement.

PEQ Editor uses Apache's verified operator identity through a small session adapter and the same constrained content DB identity. Its built-in DB administration/default-password account setup is not enabled. phpMyAdmin uses normal database login; the documented editor identity is intentionally limited.

SSH/SFTP and FTPS share only `operator-data` with `quests/` and `uploads/`. Put quest overrides in `quests/`; originals remain immutable in the verified release. A deliberate server restart merges overrides into its launch view. New uploads are not applied to the running server automatically. No server cache or operator client archives are writable through these services.

## Backups and restore

`BACKUP_SCHEDULE` uses five schedule fields (minute/hour/day/month/weekday), default `0 3 * * *`, with `TZ` and retention days configurable. The backup profile runs the scheduler inside the stack, not as a host/Codex automation. The actual schedule and selected utility versions are recorded in logs/receipts.

```sh
python3 everforge.py backup
python3 everforge.py backup-list
```

Each completed archive contains an InnoDB transactional full dump of **both** `peq_full` and `spire`, including custom tables, routines/events/triggers/views, plus server configuration/world key, custom quest overlays and Spire state/encryption key. The dedicated reader uses a private options file; DB passwords do not appear in commands or logs. Dump failure cannot publish a successful backup. Compression/checksums are validated before atomic finalization; retention only removes this tool's expired completed archive/receipt pairs.

Backups exclude release/download caches, operator original archives/maps/mesh inputs, generic uploads, native server logs and host Docker credentials. Keep the host `.env`, SSH public/private keys, `ssh-state`, `ftp-state` and utility-install directory separately as operator configuration. Local backup archives contain private DB/configuration secrets; retain them privately. There is no cloud upload.

The DB snapshot is transactionally consistent for InnoDB. For a matching DB+file snapshot, pause editing and stop the server/Spire/FTP/SSH writers before a manual backup; scheduled online backups do not claim an atomic filesystem+DB snapshot. Non-InnoDB tables cause a clear refusal.

Restore always uses a separate project and empty schemas/file destinations:

```sh
python3 everforge.py restore \
  --utility-install ../everforge-utilities \
  --target-project everforge-recovery \
  --target-directory ../everforge-recovery \
  --archive everforge-TIMESTAMP.tar.gz
```

The helper creates private target credentials, restores into a separate MariaDB instance with no host ports and preserves the source archive. It refuses an existing source project, nonempty target schemas or occupied target file directories. Preserve the target's private `.restore-root-password` file. To activate recovery, reattach the restored data/quest/Spire directories, retain the operator environment and resource mounts, provision utility identities against the target DB and verify the release pin before serving it. User/grant tables from the source MariaDB system schema are deliberately not replayed.

The isolated restore test matched the complete frozen game data/schema and Spire metadata, retained the binary probe/NPC edit, omitted a post-backup source marker, and booted restored services without reseeding. No gameplay acceptance is implied.

## Core release updates and persistence

An empty `EQEMU_RELEASE` resolves the latest stable **server** release on each start; an explicit cached tag can start offline. Changed payloads are staged and verified against exact platform asset, published checksum inventory, manifest, per-file hashes, native ABI, SQL binding and resource requirements. There is no polling watcher or forced running restart. `docker compose restart server` deliberately checks the selected policy.

Existing matching databases skip full SQL permanently, using a DB-local bootstrap marker even if the server cache is replaced. A changed SQL digest, unmanaged DB, interrupted import or schema mismatch stops before native launch. The current release provides no reviewed automatic additive upgrade chain; never replay fresh-install SQL over player data. Keep the compatible cached release pinned and obtain the reviewed migration procedure before changing DB requirements. Native `server.auto_database_updates=false` is retained. No upstream server/content updater runs over this fork.

Download/ABI/checksum failures preserve cached artifacts and DB, and fail clearly rather than silently running an incompatible or fallback server. Pin the last compatible cached release in `.env` and recreate the server deliberately to recover. Core image version, server release tag and utility version lock are distinct.

Named database/maps/utility-state volumes and configured server-data binds survive replacement. Never use `down -v` or prune these volumes to update. `everforge.py` also provides `start`, `stop`, `restart`, `logs` and a scoped operator `shell`.

## V2 server-side maps

Pinned public maps: `EQEmu/maps` commit `e8efa8ed4f4ea4c434c03c40d2a758388a54f4d6`, archive SHA256 `a0abccc76a993ea582974294fc86e565791466a4d1166a9cdc7dd2eb92b8dfec`. The public map cache preserves the original GPL license and provenance.

The complete inventory has **507 V2 collision maps** and six supported legacy V1 files: `sky.map`, `frogequip.map`, `vequip.map`, `warrens2.map`, `warslikswood_lit.map`, `firiona_lit.map`. V2 magic `0x02000000` routes to the published native `Map::LoadV2`; V1 is also supported. No verified public V2 replacement/conversion is claimed for those six exceptions.

All 497 navigation files use the supported EQNAVMESH V2 format. Water data is separate: 494 files, 234 V2 and 260 supported V1. `base/`, `nav/` and `water/` remain in the layout expected by native source. The 13 exact operator housing maps are overlaid by verified hash, never replaced with an unrelated public map. See `MAP-FORMATS.json` for full coverage/exception proof; file-format validation is distinct from gameplay/pathing acceptance.

The source bundle includes `VERIFICATION.md` with selected versions and validation limits. Design references: [AkkStack](https://docs.eqemu.dev/akk-stack/introduction/), [Spire](https://github.com/EQEmu/spire), [server maps](https://docs.eqemu.dev/server/maps/).
