# RoF2 branches on upstream develop

Every RoF2 branch starts from **upstream `develop` commit `c9c8747ac949deddb140d196d241fa4e777500c7`**. This was the latest upstream develop head verified for this rebase. No master changes or merge commits are added above that foundation. [Review the changes](COMPARE.md).

| Branch | Included source |
| --- | --- |
| `master` | Current upstream stable master; separate from the feature foundation |
| `develop` | Unmodified upstream develop at the verified base |
| `rof2-upstream-base` | Alias of that exact upstream develop commit; no custom merge |
| `rof2-shared-core` | Shared inventory/session/database infrastructure |
| `rof2-housing` | Shared core + published housing implementation and content |
| `rof2-fellowship` | Housing stack + fellowship and its housing access adapter |
| `rof2-mercenaries` | Fellowship stack + mercenaries |
| `rof2-spirit-shrouds` | Mercenary stack + Spirit Shrouds |
| `rof2-achievements` | Shared core + achievement engine and reviewed quest content |
| `rof2-spells` | Shared core with generic native task rewards + spell database/quest content; optional labelled pet/aura estimates |
| `rof2-integration` | All five published features, including achievements |

Each feature layer is a single review commit over its prerequisite. The combined branch is linear: the shroud stack followed by achievement integration. The separate achievement branch remains available; its production content is also included in the combined source. Gameplay dependencies are unchanged; these names do not imply independent cherry-picks.

The previous mixed master/develop heads are preserved under `archive/20261029-before-develop/`. They are backups, not ancestors of the rebased feature commits. Do not merge them into the new branches; that would reintroduce the old history. Use the [comparison and existing-clone instructions](COMPARE.md).

Native source patches are already applied. Historical release preimages in `extensions/rof2/` preserve provenance; do not reapply them. Existing PEQ/MariaDB databases use the required migrations and reviewed content; no database fork or private dump is required. Unfinished unpublished quest research remains excluded.

The spells branch preserves its original published content commit and merges shared-core revision `fc88a0ba79a05e437e7b61af4e38ead32131de0f`; it does not inherit the achievement feature. The merge preserves both published histories without rewriting either.
