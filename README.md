# EQEmu · RoF2 Achievements

Current EQEmu source with **achievements** integrated on the common upstream foundation.

**[Install / upgrade](docs/rof2/INSTALL.md)** · **[Opcodes and packet structures](docs/rof2/PROTOCOL.md)** · **[Choose a feature branch](docs/rof2/BRANCHES.md)**

This is the separate achievement feature branch. The same engine, reviewed quests, rewards, opcodes and structures are also included in [rof2-integration](https://github.com/nightstalker6669/EQEmu/tree/rof2-integration).

All feature branches now start directly from **upstream develop `c9c8747a`**, without the earlier master/develop merge history. [Review a clean diff or update an existing clone](docs/rof2/COMPARE.md). `master` remains separate.

Native source is already merged into the normal EQEmu directories. Required SQL, quest/content tooling, opcode and structure references, attribution and evidence limits are retained in `extensions/rof2/`. Follow this branch’s installation guide; its historical release patches must not be reapplied. Feature dependencies and reconstructed/default policies remain documented.

Use an existing PEQ/MariaDB database with the required upstream and feature migrations; a separate PEQ fork is unnecessary. No unfinished quest research, private runtime records, client assets or custom test suites are shipped. Publication does not deploy the running server or establish native gameplay acceptance of this new combination.

[Current source versions and validation](docs/rof2/integration.json) · [Upstream project documentation](docs/rof2/UPSTREAM-README.md) · [GPLv3](LICENSE.md)
