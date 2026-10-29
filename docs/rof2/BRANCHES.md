# Current RoF2 branches

All feature branches use the same [current upstream foundation](https://github.com/nightstalker6669/EQEmu/tree/rof2-upstream-base): upstream stable `master` **`4aceae18`** plus the current protocol-development foundation **`c9c8747a`**. The merge is pinned at **`d03e3292cdc30c531e307db5b2f049c0d2df0460`**. These were the upstream heads verified for this update; this is a fork integration, not an official new upstream release.

| Branch | Included source |
| --- | --- |
| [master](https://github.com/nightstalker6669/EQEmu/tree/master) | Current upstream stable master; no custom feature integration |
| [rof2-upstream-base](https://github.com/nightstalker6669/EQEmu/tree/rof2-upstream-base) | Common stable + development foundation |
| [rof2-shared-core](https://github.com/nightstalker6669/EQEmu/tree/rof2-shared-core) | Shared inventory/session/database infrastructure |
| [rof2-housing](https://github.com/nightstalker6669/EQEmu/tree/rof2-housing) | Shared core + complete published housing release |
| [rof2-fellowship](https://github.com/nightstalker6669/EQEmu/tree/rof2-fellowship) | Housing + fellowship and its housing access adapter |
| [rof2-mercenaries](https://github.com/nightstalker6669/EQEmu/tree/rof2-mercenaries) | Fellowship stack + mercenaries |
| [rof2-spirit-shrouds](https://github.com/nightstalker6669/EQEmu/tree/rof2-spirit-shrouds) | Mercenary stack + Spirit Shrouds |
| [rof2-achievements](https://github.com/nightstalker6669/EQEmu/tree/rof2-achievements) | Shared core + achievement engine and reviewed quest content; its own feature branch |
| [rof2-integration](https://github.com/nightstalker6669/EQEmu/tree/rof2-integration) | **All five features, including achievements**, on the common foundation |

Choose **rof2-integration** to install everything together. Achievements remains available separately for operators who want that feature branch. Its engine is now included in the combined source; there is no older-base exclusion or separate-base requirement.

The other feature branches retain their documented gameplay dependencies. A branch name does not imply its diff is an independent cherry-pick. Original release patches in `extensions/rof2/` preserve their historical preimages; do not reapply those patches to these already integrated branches.

The original 23.10.3 commit `8175ae61874cbf20d7b05102821f6f8a4f4d1c39` remains in history. An existing PEQ/MariaDB database plus the required upstream and feature migrations is sufficient; no PEQ fork or private database dump is required. Unfinished unpublished quest research remains excluded.
