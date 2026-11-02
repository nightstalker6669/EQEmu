# RoF2 branches on upstream develop

Every RoF2 branch starts from **upstream `develop` commit `c9c8747ac949deddb140d196d241fa4e777500c7`**. This was the latest upstream develop head verified for this rebase. No master changes or merge commits are added above that foundation. [Review the changes](COMPARE.md).

| Branch | Included source |
| --- | --- |
| `master` | Current upstream stable master; separate from the feature foundation |
| `develop` | Unmodified upstream develop at the verified base |
| `rof2-upstream-base` | Alias of that exact upstream develop commit; no custom merge |
| `rof2-shared-core` | Shared inventory/session/database, task rewards, transport and shared-task admission infrastructure |
| `rof2-housing` | Shared core + published housing implementation and content |
| `rof2-fellowship` | Housing stack + fellowship and its housing access adapter |
| `rof2-mercenaries` | Fellowship stack + mercenaries |
| `rof2-spirit-shrouds` | Mercenary stack + Spirit Shrouds |
| `rof2-achievements` | Shared core + achievement engine and reviewed quest content |
| `rof2-integration` | All five published features, including achievements |

The achievement and combined integration source layers are linear consumers of published shared-core commit `17d63f7d512711e911787e630f4533ee14a1e785` (schema 9335). The combined layer includes housing, fellowship, mercenaries, Spirit Shrouds and achievements in its source tree. Generic task rewards, carried Collect, completion gates, transport and shared-task ownership require this core; they do not require the achievement engine. Achievement progress, criteria, providers and achievement-specific rewards remain on their feature layer. Other feature branches must adopt their documented core dependency before combining newer migrations.

The previous mixed master/develop heads are preserved under `archive/20261029-before-develop/`. They are backups, not ancestors of the rebased feature commits. Do not merge them into the new branches; that would reintroduce the old history. Use the [comparison and existing-clone instructions](COMPARE.md).

Native source patches are already applied. Historical release preimages in `extensions/rof2/` preserve provenance; do not reapply them. Existing PEQ/MariaDB databases use the required migrations and reviewed content; no database fork or private dump is required. Unfinished unpublished quest research remains excluded.
