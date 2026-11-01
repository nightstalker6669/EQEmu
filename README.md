# EQEmu · RoF2 Shared Core

Current EQEmu source with **shared inventory, session, task reward and database infrastructure** integrated on the common upstream foundation.

**[Install / upgrade](docs/rof2/INSTALL.md)** · **[Opcodes and packet structures](docs/rof2/PROTOCOL.md)** · **[Choose a feature branch](docs/rof2/BRANCHES.md)**

All feature branches now start directly from **upstream develop `c9c8747a`**, without the earlier master/develop merge history. [Review a clean diff or update an existing clone](docs/rof2/COMPARE.md). `master` remains separate.

Native infrastructure is already merged into the normal EQEmu directories. This core branch contains the shared implementation and additive schema; feature branches add their own production SQL, quests, opcode and structure references, and installation guides. Follow each selected branch’s installation guide.

The core itself does not enable feature gameplay or create properties/accounts. [Core API and persistence boundaries](common/shared_core/README.md) describe the legacy identifiers retained for compatibility.

[Current source versions and validation](docs/rof2/integration.json) · [Upstream project documentation](docs/rof2/UPSTREAM-README.md) · [GPLv3](LICENSE.md)
