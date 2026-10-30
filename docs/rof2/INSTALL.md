# Install the shared core

## Current upstream foundation

All feature branches now start directly from upstream `develop` commit **`c9c8747ac949deddb140d196d241fa4e777500c7`**, with no master merge in their new history. `master` remains the separate stable upstream branch. See [the branch map](BRANCHES.md), [clean comparisons and clone updates](COMPARE.md), and [exact provenance](integration.json).

Before applying feature migrations to an older database, build the matching current `world` binary through the normal upstream/vcpkg workflow and run its database updater from the configured server directory (`world database:updates`, or `world.exe database:updates` on Windows). Back up first and keep game services stopped. Review and complete the updater before the feature installation below. The housing migration ledger is not a replacement for upstream schema upgrades.

Current upstream uses vcpkg, CMake 3.20+, and a C++20 compiler. Initialize submodules before configuring. For native Windows, use the current upstream Visual Studio/vcpkg toolchain; for Akk-stack, use a build container with those dependencies and the correct mounts. System libraries alone are not a substitute for the current dependency manifest.

Build through the normal upstream workflow. The core alone does not activate the feature session hooks. Its additive `utils/sql/shared_core/001_inventory_sessions.sql` preserves the original durable table names; follow the [core boundaries](../../common/shared_core/README.md) before provisioning it. Feature branches already inherit this code and apply their required schema through their documented migration path.

[Branch map](BRANCHES.md) · [Provenance](integration.json)
