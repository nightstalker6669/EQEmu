# EQEmu · RoF2 Achievements

Ready-to-build EQEmu source with **achievements** integrated from the published production releases. All required companion source, SQL, content tooling and protocol references are included.

**[Installation and database order](docs/rof2/INSTALL.md)** · **[Opcodes and packet structures](docs/rof2/PROTOCOL.md)** · [Source versions](docs/rof2/integration.json)

This is the `rof2-achievements` branch. Each feature has its own branch; the published dependency stack means fellowship includes housing, mercenaries includes both, and shrouds includes all three. Achievements keeps its separate newer upstream base. `master` remains the original 23.10.3 release. These are documented implementation dependencies rather than a claim that the features are independent patches.

Native source is in the normal EQEmu directories. `extensions/rof2/` holds the corresponding curated release packages: ordered migrations, installers, quest scripts, opcodes, structures, evidence limits and attribution. Their old source patch commands are for reproducing original releases; **do not reapply them to this already-integrated source**.

An existing PEQ/MariaDB database plus the included migrations is sufficient; no PEQ fork or private database dump is required. Read the setup guide before installing. This source integration was checked offline; the combined branch still needs a fresh build and native deployment acceptance. No running server was changed.

[Upstream build/project documentation](docs/rof2/UPSTREAM-README.md) · [GPLv3 license](LICENSE.md)

[All feature branches and dependency order](docs/rof2/BRANCHES.md)
