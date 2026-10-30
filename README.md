# EQEmu · RoF2 Integration

Current EQEmu source with **all five published features: housing, fellowship, mercenaries, Spirit Shrouds and achievements** integrated on the common upstream foundation.

**[Install / upgrade](docs/rof2/INSTALL.md)** · **[Opcodes and packet structures](docs/rof2/PROTOCOL.md)** · **[Choose a feature branch](docs/rof2/BRANCHES.md)**

Achievements is included here alongside housing, fellowship, mercenaries and shrouds. It also remains available on its own [rof2-achievements branch](https://github.com/nightstalker6669/EQEmu/tree/rof2-achievements).

All feature branches now start directly from **upstream develop `c9c8747a`**, without the earlier master/develop merge history. [Review a clean diff or update an existing clone](docs/rof2/COMPARE.md). `master` remains separate.

Native source is already merged into the normal EQEmu directories. Required SQL, quest/content tooling, opcode and structure references, attribution and evidence limits are retained in `extensions/rof2/`. Follow this branch’s installation guide; its historical release patches must not be reapplied. Feature dependencies and reconstructed/default policies remain documented.

Use an existing PEQ/MariaDB database with the required upstream and feature migrations; a separate PEQ fork is unnecessary. No unfinished quest research, private runtime records, client assets or custom test suites are shipped. The separate 2026-10-30 controlled migration completed a full Linux combined build/deployment and representative RoF2 checks with housing/player data preserved. Six Roost quests, bound-nest melee/lifecycle, feather rewards and the class-filtered paired-earring claim passed. This is scoped evidence, not complete cross-feature or native Windows acceptance. Publication itself does not deploy a server.

Later achievement milestones include [Mesa `a21ffd63`](https://github.com/nightstalker6669/eqemu-rof2-achievements/tree/a21ffd63d1fe527b4a2d689b081228e5a8221cf4) and [Roost `2b7d6944`](https://github.com/nightstalker6669/eqemu-rof2-achievements/tree/2b7d69447aa8a2f998eb1a18fbbb60394074e7e6). Unresolved Roost Ascending wrist and Raider glove/bundle rewards remain unconfigured. Original input provenance is retained in the manifest.

[Current source versions and validation](docs/rof2/integration.json) · [Upstream project documentation](docs/rof2/UPSTREAM-README.md) · [GPLv3](LICENSE.md)
