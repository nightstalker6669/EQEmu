# EQEmu · RoF2 integrated feature stack

Complete source for the published **housing, fellowship, mercenary and spirit shroud** releases, built on the extracted shared core and EQEmu 23.10.3.

**[Install the complete stack](docs/rof2/INSTALL.md)** · **[Opcodes and packet structures](docs/rof2/PROTOCOL.md)** · **[All branches and dependencies](docs/rof2/BRANCHES.md)**

The source is already integrated. Required SQL, quest/content tools, opcode/structure references, original release provenance and platform-specific prerequisites are included in `extensions/rof2/`. Use the integration-aware housing installer and then the documented companion migration order; do not reapply source overlays or start game processes midway through the upgrade. An existing PEQ/MariaDB database is sufficient; no PEQ fork is required.

The shared core library and all eight server/housing executable targets passed Linux builds. Critical lifecycle files also passed syntax checks in the smaller feature branches. Native deployment/gameplay acceptance and native Windows execution were not performed for this integration.

**Achievements is deliberately separate:** [rof2-achievements](https://github.com/nightstalker6669/EQEmu/tree/rof2-achievements) includes its published nine-patch engine, reviewed quests, rewards, opcodes and structs on newer upstream `c9c8747`. It is not merged into this older-base stack. See the branch map before combining code across those bases.

`master` remains the exact original 23.10.3 commit. No running server, client or database was changed by this publication. Feature limitations and labelled reconstructed/default values remain documented in each release; this integration does not claim complete Live parity.

[Integration provenance and validation](docs/rof2/integration.json) · [Upstream project/build documentation](docs/rof2/UPSTREAM-README.md) · [GPLv3](LICENSE)
