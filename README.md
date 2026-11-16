# EQEmu RoF2 Restoration Pack

**Bringing more of the Rain of Fear experience to EQEmu, one complete system and quest line at a time.**

This pack builds on EQEmu and ProjectEQ to restore features, content, and progression for the **May 10, 2013 Rain of Fear 2 client**. It combines server changes, quest scripts, and database additions so restored systems can work together.

The aim is familiar EverQuest gameplay: build a home, gather your fellowship, hire a mercenary, earn achievements, learn spells through their original acquisition routes, and craft with recipes from across the expansions through Rain of Fear.

Restoration is ongoing. The pack includes substantial working additions, but it does not yet recreate every quest, spell acquisition route, AA, or encounter through RoF.

**[EverForge on Docker Hub](https://hub.docker.com/r/nightsta69/everforge-rof2)** · **[Docker installation](docs/rof2/EVERFORGE.md)** · **[Server downloads](https://github.com/nightstalker6669/EQEmu/releases/latest)** · **[Database downloads](https://github.com/nightstalker6669/EQEmu-Database/releases)** · **[Installation guide](docs/rof2/INSTALL.md)** · **[Feature source and documentation](extensions/rof2/)**

## What this pack adds

| Area | What the restoration brings |
| --- | --- |
| **Player and guild housing** | Sunrise Hills neighborhoods, purchasable plots, houses and guild halls, decoration, saved layouts, storage, upkeep, permissions, and guild services. [Details](extensions/rof2/housing/) |
| **Fellowships** | Fellowship creation and invitations, the native window and chat, campsites and their effects, insignia travel, and normal/AA experience sharing. [Details](extensions/rof2/fellowship/) |
| **Mercenaries through level 100** | Tank, healer, melee, and caster roles; hiring, upkeep, stances, suspension, revival, and extended level 86–100 stats and spell progression. Group resource updates also reflect changing mana. Some scaling values are documented reconstructions. [Details](extensions/rof2/mercenaries/) |
| **Oceangreen progression** | Ten accepted personal/shared tasks, once-only shared rewards, earned Void B access and ordinary Journeyman II acquisition. Upper-ward physical blocking remains limited; encounter values are documented local reconstruction. [Scope and upgrade guide](docs/rof2/OCEANGREEN.md) |
| **Kithicor, Void C and Journeyman III** | Six accepted progression tasks and shared mission rewards, earned original Void C access, spell-vendor acquisition and an ordinary Journeyman III healer hire. Physical escorts passed on the accepted native cohort; a later guard fix has focused validation only. [Scope and upgrade guide](docs/rof2/KITHICOR.md) |
| **Spirit Shrouds** | Native selection and previews, transformation and character restoration, earned-item retrieval, and shroud abilities. The enabled catalogue covers eight starting forms at levels 5–70; later branches remain incomplete. [Details](extensions/rof2/spirit-shrouds/) |
| **Achievements and quest rewards** | Achievement tracking, persistent progress, completion notifications, and reward selection, alongside restored portions of Hero’s Journey and Rain of Fear quest content. Coverage includes work in Crescent Reach, Blightfire Moors, Stone Hive, and Shard’s Landing. [Details](extensions/rof2/achievements/) |
| **Spells and their acquisition routes** | Missing scrolls and vendor stock, faction and language requirements, and original rune, fragment, mote, cantrip, and periapt exchanges. The work covers multiple expansions through RoF; remaining access and quest gaps are tracked separately. [Details](extensions/rof2/spells/) |
| **Alternate Abilities** | Reviewed AA data corrections and targeted support for eligibility and progression. This includes published Origin and knight innate work, with further AA coverage still being audited. [Details](extensions/rof2/aa/) |
| **Tradeskills** | Recipe and ingredient additions, corrected combines, and support for special crafting and quest hand-ins across the through-RoF recipe audit. Historical uncertainties and unfinished recipes remain documented. [Details](extensions/rof2/tradeskills/) |

These additions share improvements to inventory handling, purchases, task credit, and reward delivery. Feature documentation explains the supported scope and remaining limitations; inclusion here is not a claim that every variation has been tested.

## Restoration in progress

The accepted Oceangreen slice now covers the ten-task village and Blackburrow progression batch, shared rewards, earned Void B access, scroll acquisition and ordinary Journeyman II hiring. Use the matching server/database release and [reviewed additive upgrade](docs/rof2/OCEANGREEN.md); the full database export is for fresh installations only. Upper-ward closed/open collision remains limited and is not a global pass.

The accepted Kithicor batch extends this progression through six tasks, earned original Void C access, spell acquisition and an ordinary Journeyman III healer hire. See the [Kithicor scope and upgrade guide](docs/rof2/KITHICOR.md) for the native escort evidence and the limits on the later guard fix.

Further Seeds of Destruction themes, spell acquisition gaps, AA coverage, achievement content and unresolved recipes remain unfinished. Completed improvements move into the feature overview and release notes with their accepted scope.

## EverForge Docker stack

EverForge0.3.0 runs the published RoF2 server with our source-built Spire and PEQ forks, real World administration, SSH/SFTP, FTPS, phpMyAdmin and complete private backup/restore. See [the full installation and upgrade guide](docs/rof2/EVERFORGE.md) and [ports/network access](docs/rof2/everforge/PORTS.md). Required authorized original EQGs/maps are supplied separately; they are not distributed in the image.

```sh
mkdir everforge
docker run --rm --entrypoint tar nightsta69/everforge-rof2:0.3.0 \
  -C /opt/everforge/setup -cf - . | tar -xf - -C everforge
cd everforge
python3 configure.py
```

Continue with the full guide for resources, utility installation, initialization and credentials. Existing installations retain their database/data volumes and private environment. Utilities use the reviewed fork commits recorded in the setup; ordinary restart does not reinstall upstream software or reseed an existing DB. [Docker Hub](https://hub.docker.com/r/nightsta69/everforge-rof2) · [validation](docs/rof2/everforge/VERIFICATION.md).

## Getting the pack

Start with the [server releases](https://github.com/nightstalker6669/EQEmu/releases/latest). Choose the package for the computer running your server, then follow its included installation instructions and matching database reference.

The [companion database repository](https://github.com/nightstalker6669/EQEmu-Database) provides database provenance, additions, and downloadable snapshots. Use the database version specified by your server package. A full database snapshot is for a **fresh installation**; it must not be imported over an existing player database. Existing servers need the appropriate migrations.

Development takes place on [`develop`](https://github.com/nightstalker6669/EQEmu/tree/develop). Release notes identify the source and content included in each downloadable package.

## Keeping the restoration clear

The goal is to follow the original game where reliable evidence is available. When original values or mechanics cannot be recovered, local reconstruction choices are documented rather than presented as exact historical data. Working features, unfinished content, and items awaiting gameplay testing are kept distinct.

This overview is updated with completed restoration batches. The release notes provide the dated record of what changed and which download contains it.

## Built on the community’s work

This project builds on [EQEmu](https://github.com/EQEmu/EQEmu), [ProjectEQ](https://github.com/ProjectEQ), and the work of their contributors. Credit for EverQuest and its original art and game content belongs to its creators and rights holders.

[Upstream EQEmu overview](docs/rof2/UPSTREAM-README.md) · [EQEmu documentation](https://docs.eqemu.dev/) · [License](LICENSE.md)

