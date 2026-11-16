# Kithicor progression, Void C and Journeyman III

This batch extends the published Oceangreen progression with six Kithicor tasks, their personal and shared mission mechanics, earned access to the original Void C, spell acquisition and Journeyman III hiring. It combines the matched native server changes, eighteen additional quests and sanitized static database content. Existing Oceangreen quests and previously published features remain included.

## Accepted gameplay

Charges of Treason, Vengeance and Deliverance, Destroy the Impostor, Rain of Ruin, Safe Passage and Bitter Victuals completed on the matched local native server, with all thirty-one retained reward components delivered. Shared reward delivery was checked for two real recipients. Both physical escort routes, the refugees and their rewards passed in the accepted native run.

Earned progression opened the original Void C entrance and its twenty-two vendor rows. An actual 165 Chronobine purchase of the Mindfreeze scroll followed by native scribing succeeded, and the previously learned Oceangreen spell remained available. An ordinary level-100 Journeyman III healer hire succeeded while the existing mercenaries remained owned. These are the observed acquisition paths; they do not establish acceptance for every class, mercenary role or encounter variation.

The matched public content enables the Kithicor slice after its required schemas and content are installed. The four new private runtime tables are distributed as empty schemas. Published content excludes player records, QA state and private operational data. The development damage-cap override is excluded; the public normal default remains zero.

## Qualifications

The physical escort acceptance used the r8 quest revision. The final r11 guard-homing fix was validated through focused mocks and exact file staging for a future fresh load. A full native escort walk with r11 was not repeated and is not claimed here. The causes of the earlier 615/616 guard behavior remain unproven; this is not acceptance of every guard route.

The owned AA exit and the main NPC ready path were observed. The peer character's NPC ready path remains unproven. Mission numbers and encounter values are the approved local reconstruction, not a claim of exact historical Live parity.

Native gameplay acceptance used the matched local Linux cohort. Windows/Linux CI builds, tests and packaging establish their stated build results, not native gameplay acceptance on another platform. The existing [Oceangreen qualifications](OCEANGREEN.md), including limited upper-ward physical blocking, continue to apply.

## Installation and existing player databases

Use the matched server release, pinned database archive and included installation guide together. The release applies thirty-three pinned quests: the existing fifteen Oceangreen files and the eighteen Kithicor additions. Keep the quest set, native binaries and database version matched; retain operator configuration and unrelated quest files. Original authenticated RoF2 resources and maps remain operator supplied.

The sanitized full SQL is for a **new empty database only**. Never import it over a player database. Existing installations must follow `database/migrations/kithicor/UPGRADE-CONTRACT.json` and its ordered migrations shipped with the matched database release. The supported starting point is the published Oceangreen database lineage; other lineages or an occupied feature namespace require their own reviewed disposition.

Stop game and administrative writers and retain a matched backup before the upgrade. Apply `001_kithicor_required.sql` in a reviewed MariaDB session with `@kithicor_20261115_authorized=1`. Only after its postconditions succeed, managed EverForge installations apply `002_everforge_accept_seed.sql` with `@kithicor_20261115_marker_authorized=1` for the exact seed-marker transition. Source copies of these inputs and the contract are under `utils/sql/sod/kithicor/`. The class-only correction fragment is not a substitute for installing an absent Kithicor feature. Preserve player-earned state and unrelated custom content. Do not weaken guards, advance the marker by hand, use a client force option or blindly retry partially committed DDL.

After the successful reviewed database upgrade, deliberately install the matching project server release through Spire or the documented stopped-container CLI path. An ordinary EverForge restart retains the installed binary selection and does not upgrade it. Never replay the full seed to bypass an upgrade refusal. See the [EverForge installation guide](https://github.com/nightstalker6669/EQEmu/blob/develop/docs/rof2/EVERFORGE.md).
