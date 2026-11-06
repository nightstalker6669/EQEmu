# Optional CopyCharacter journal safeguards

The shared-core proposal excludes selected journal rows from copying and checks the allocated destination for orphan event/generation rows before copying. A registered journal with an unavailable schema refuses the copy and rolls back; no journal identity is erased or re-keyed.

The feature opts in through the character table registry. Unregistered journal tables are not queried, so neutral shared-core does not require achievement schema. The achievement layer registers its two journal tables and owns schema9339. This registry gate is a new source adaptation of the integrated candidate and has static review only. The public baseline has not been compiled or run. The separate five-case fixture is not execution evidence for this gate. NULL serialization is a separate published prerequisite.

No backup/restore/delete batch, concurrent ID safety, native acceptance, deployment or activation is claimed. Publish this proposal only after source review; schema registration, matching engine and all activation prerequisites must be coordinated separately.
