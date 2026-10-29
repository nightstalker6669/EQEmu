# Install `rof2-achievements`

This checkout already contains the native source integration. **Do not run `apply_overlay.py` or `git apply` again.** Keep backups of the database, quests, binaries and active opcode configuration together before installing. No running server was modified by publishing this branch.

```sh
git -c core.autocrlf=false clone --branch rof2-achievements --single-branch https://github.com/nightstalker6669/EQEmu.git
cd EQEmu
git submodule update --init --recursive
```

## Current upstream foundation

All feature branches now share the same merged upstream base. `master` follows the verified current stable upstream; the common feature foundation also includes the protocol-development changes needed by achievements. See [the branch map](BRANCHES.md) and [exact provenance](integration.json).

Before applying feature migrations to an older database, build the matching current `world` binary through the normal upstream/vcpkg workflow and run its database updater from the configured server directory (`world database:updates`, or `world.exe database:updates` on Windows). Back up first and keep game services stopped. Review and complete the updater before the feature installation below. The housing migration ledger is not a replacement for upstream schema upgrades.

Current upstream uses vcpkg, CMake 3.20+, and a C++20 compiler. Initialize submodules before configuring. For native Windows, use the current upstream Visual Studio/vcpkg toolchain; for Akk-stack, use a build container with those dependencies and the correct mounts. System libraries alone are not a substitute for the current dependency manifest.

## Engine, database and quests

The nine patches in the [engine guide](../../extensions/rof2/achievements/ENGINE.md) are already applied. Follow its **database migration and deployment** directions, skipping the source patch commands. Apply `2026_07_28_achievement_content.sql`, `2026_07_28_achievement_state.sql`, then `2026_10_27_achievement_reward_classes.sql` from the integrated source. The first two are registered with the pinned updater; inspect existing schema/history before any replay.

Install **all reviewed content** in the order given in the [release installation section](../../extensions/rof2/achievements/README.md#install), running each package's guarded preflights and quest installer and importing every required SQL file. This includes Crescent, Shard’s Landing, all five Blightfire quest packages, Stone Hive, Blightfire parent rewards and Mesa. Keep unknown conflicting quest scripts for a deliberate merge. Do not import research catalogues as playable content.

Build and deploy matching world/zone binaries and `utils/patches/patch_RoF2.conf` through your normal Linux, native Windows or Akk-stack workflow. The [Mesa guide](../../extensions/rof2/achievements/content/mesa/README.md) includes native Windows and Akk-stack examples. Achievements now shares the same upstream foundation as the other feature branches and is also included in the combined branch.

## References

[Achievement engine and provenance](../../extensions/rof2/achievements/ENGINE.md)

[Opcodes and structures](PROTOCOL.md) · [Integration manifest](integration.json) · [Original upstream README](UPSTREAM-README.md)

## Verification

See [integration.json](integration.json) for validation of the current source. Original release evidence is retained separately. No live server or database was changed during publication.
