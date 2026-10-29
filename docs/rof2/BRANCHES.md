# Published branch map

| Branch | Contents / prerequisite |
| --- | --- |
| [master](https://github.com/nightstalker6669/EQEmu/tree/master) | Exact original EQEmu 23.10.3 (`8175ae6`); unchanged |
| [rof2-shared-core](https://github.com/nightstalker6669/EQEmu/tree/rof2-shared-core) | Reusable inventory/session/database infrastructure; no feature gameplay |
| [rof2-housing](https://github.com/nightstalker6669/EQEmu/tree/rof2-housing) | Shared core + complete published housing release |
| [rof2-fellowship](https://github.com/nightstalker6669/EQEmu/tree/rof2-fellowship) | Housing branch + fellowship and housing access adapter |
| [rof2-mercenaries](https://github.com/nightstalker6669/EQEmu/tree/rof2-mercenaries) | Fellowship branch + mercenary release |
| [rof2-spirit-shrouds](https://github.com/nightstalker6669/EQEmu/tree/rof2-spirit-shrouds) | Mercenary branch + shroud release and cross-feature restoration |
| [rof2-integration](https://github.com/nightstalker6669/EQEmu/tree/rof2-integration) | Combined 23.10.3 feature stack, with setup/index documentation |
| [rof2-achievements](https://github.com/nightstalker6669/EQEmu/tree/rof2-achievements) | Published nine-patch achievement engine + reviewed content on newer upstream `c9c8747` |

Each branch is a complete checkout. The feature branches retain their published gameplay/packaging dependencies; their names do not mean their diffs can all be cherry-picked independently. The common inventory/session implementation is now inherited from the core, with legacy schema identifiers retained for compatibility.

Achievements deliberately remains on its published newer base. It is not included in the 23.10.3 integration branch, and merging it there still requires a separately reviewed base upgrade or backport. Unfinished unpublished quest research is excluded.

Every feature branch includes its required production SQL, release tooling, protocol references, structures, attribution and documented limits. Use that branch's installation guide. An existing PEQ/MariaDB database plus these migrations is sufficient; a separate PEQ fork is unnecessary. Backups, matching dependencies and operator-owned original client resources remain prerequisites.
