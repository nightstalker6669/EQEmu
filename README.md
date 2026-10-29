# EQEmu · RoF2 shared core

Reusable inventory/session and database infrastructure for the RoF2 feature branches, based on EQEmu 23.10.3. **No housing gameplay, fellowship, mercenaries, shrouds or achievements are enabled by this branch.**

- [Core boundaries, API and migration requirements](common/shared_core/README.md)
- [Provenance](docs/rof2/integration.json)
- [EQEmu upstream project and build instructions](https://github.com/EQEmu/EQEmu/tree/8175ae61874cbf20d7b05102821f6f8a4f4d1c39)

Feature branches inherit this core and document their additional source/database dependencies. `master` stays at the original 23.10.3 commit. Achievements retains its separate newer upstream base.

Build through the normal EQEmu CMake workflow. The core session migration is additive; it is not a PEQ database replacement or a full feature installer. Preserve database/binary backups and existing durable lease history. No test accounts, logs, private settings or client files are included. See [GPLv3](LICENSE).

[All feature branches and dependency order](docs/rof2/BRANCHES.md)
