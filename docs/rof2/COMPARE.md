# Review the spirit-shrouds changes

Base: upstream **develop `c9c8747ac949deddb140d196d241fa4e777500c7`**. [Open the full GitHub comparison](https://github.com/nightstalker6669/EQEmu/compare/c9c8747ac949deddb140d196d241fa4e777500c7...rof2-spirit-shrouds). This comparison contains feature work and its production package/documentation; it does not include the previous master/develop merge history.

## Review by scope

Fetch the fork and inspect native code and opcode mappings separately from release packaging:

```sh
git fetch origin
git diff --stat c9c8747ac949deddb140d196d241fa4e777500c7..origin/rof2-spirit-shrouds
git diff c9c8747ac949deddb140d196d241fa4e777500c7..origin/rof2-spirit-shrouds -- common zone world utils/patches
git diff c9c8747ac949deddb140d196d241fa4e777500c7..origin/rof2-spirit-shrouds -- utils/sql
git diff c9c8747ac949deddb140d196d241fa4e777500c7..origin/rof2-spirit-shrouds -- docs/rof2 extensions/rof2
```

The last group includes production SQL, quests, installation tools, protocol evidence and historical standalone patch files. Some release packages retain copies of native source/patch preimages for provenance; those are not additional compile units. Current native code in `common/`, `zone/`, `world/` and active mappings is authoritative.

Confirm the history when comparing:

```sh
git merge-base c9c8747ac949deddb140d196d241fa4e777500c7 origin/rof2-spirit-shrouds
git log --oneline c9c8747ac949deddb140d196d241fa4e777500c7..origin/rof2-spirit-shrouds
git rev-list --count --merges c9c8747ac949deddb140d196d241fa4e777500c7..origin/rof2-spirit-shrouds
```

The merge base is `c9c8747ac949deddb140d196d241fa4e777500c7` and the merge count is **0**. Feature layers are linear review commits. For a layer-only diff, compare its branch with the prerequisite listed in [BRANCHES.md](BRANCHES.md).

## Update an existing clone

The published feature history was rewritten for this requested rebase. **Do not pull/merge the old branch into the new one.** Preserve local commits and uncommitted changes first. A fresh clone of `rof2-spirit-shrouds` is the simplest option. In an existing clean clone, make a local backup branch, fetch, then create a new local branch from `origin/rof2-spirit-shrouds`. Deliberately cherry-pick only your own later custom work after reviewing it.

The prior published head is [`73065eb61e54fc34851046e3ce1e6d0ede948560`](https://github.com/nightstalker6669/EQEmu/commit/73065eb61e54fc34851046e3ce1e6d0ede948560) and is preserved by `archive/20261029-before-develop/rof2-spirit-shrouds`. No running server, clients or database are updated by this source rebase.
