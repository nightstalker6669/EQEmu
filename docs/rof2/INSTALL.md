# Install `rof2-achievements`

This checkout already contains the native source integration. **Do not run `apply_overlay.py` or `git apply` again.** Keep backups of the database, quests, binaries and active opcode configuration together before installing. No running server was modified by publishing this branch.

```sh
git -c core.autocrlf=false clone --branch rof2-achievements --single-branch https://github.com/nightstalker6669/EQEmu.git
cd EQEmu
git submodule update --init --recursive
```

Use the normal EQEmu dependencies/build workflow for your platform. Keep builds and private backups outside the source checkout. [`master`](https://github.com/nightstalker6669/EQEmu/tree/master) stays at release 23.10.3. This branch starts at `c9c8747ac949deddb140d196d241fa4e777500c7`.

## Engine, database and quests

The nine patches in the [engine guide](../../extensions/rof2/achievements/ENGINE.md) are already applied. Follow its **database migration and deployment** directions, skipping the source patch commands. Apply `2026_07_28_achievement_content.sql`, `2026_07_28_achievement_state.sql`, then `2026_10_27_achievement_reward_classes.sql` from the integrated source. The first two are registered with the pinned updater; inspect existing schema/history before any replay.

Install **all reviewed content** in the order given in the [release installation section](../../extensions/rof2/achievements/README.md#install), running each package's guarded preflights and quest installer and importing every required SQL file. This includes Crescent, Shard’s Landing, all five Blightfire quest packages, Stone Hive, Blightfire parent rewards and Mesa. Keep unknown conflicting quest scripts for a deliberate merge. Do not import research catalogues as playable content.

Build and deploy matching world/zone binaries and `utils/patches/patch_RoF2.conf` through your normal Linux, native Windows or Akk-stack workflow. The [Mesa guide](../../extensions/rof2/achievements/content/mesa/README.md) includes native Windows and Akk-stack examples. This branch preserves the newer published achievement base; it is not a compatible cherry-pick onto the 23.10.3 housing branches.

## References

[Achievement engine and provenance](../../extensions/rof2/achievements/ENGINE.md)

[Opcodes and structures](PROTOCOL.md) · [Integration manifest](integration.json) · [Original upstream README](UPSTREAM-README.md)
