# Verification scope

The deployed database additions were read back and checked against their plans. Kerkaz's 160 class/tier projections matched the complete grouped reward catalogue; all reward items resolved to existing spells with the expected class level and rank. Lua 5.1 syntax and a hail/keyword selector harness passed. The script uses the native task delivery/reward path and has no scripted reward summoning or trade handler.

The public installer was exercised against an isolated MariaDB schema populated only with required baseline game content. Planning, full overlay application, identical no-op rerun and refusal of a conflicting item row passed. The same application passed without housing-specific item columns. No account, character, player inventory or housing state tables were copied into that exercise. Public files omit the private fixtures, connection files and raw test output.

Before stage 005, a read-only installer plan against the installed content reported no new rows or vendor changes. Stage 005 and its schema/binary changes have not been deployed during the coordinated achievement test window. Publication checks include Python syntax, payload/SQL consistency, Lua syntax, Git whitespace checks and an allowlisted spell-only file inventory.

These checks do not establish native in-game turn-in acceptance, exact Live pet/aura balance, full spell engine correctness, or native Windows/Akk-stack execution. A dedicated native mote test and coordinated cache activation remain pending. Publishing this branch does not reload or restart a server.

## Shared-core integration

This branch inherits the published core revision `17d63f7d512711e911787e630f4533ee14a1e785`. The achievements maintainer reported successful world/zone linking and 386 wire-compatibility cases/task contracts for that exact core tree. The spell integration retains its source, protocol and migration bytes unchanged; the spell layer additionally implements merchant language eligibility and migration 9336. That inherited validation is separate from native gameplay acceptance of Kerkaz, which remains pending. The shared-core extraction was performed and published by the achievements maintainer, not duplicated in the spell layer.

## Merchant language validation

The merchant eligibility predicate passed 282,800 language/threshold/learned-skill cases plus default, exact-boundary and invalid-data cases. The actual normal-purchase guard and hidden-slot listing prefix were extracted into an isolated harness: below-threshold and stale-slot requests returned before the simulated debit/fallback boundary, exact-threshold purchases passed, unrestricted stock remained available, and hidden permanent slots were reserved away from player resale stock. These are offline contracts, not native gameplay tests.

The complete content installer, including stage 005, passed temporary MariaDB schema application, identical no-op rerun, missing-migration rejection, customized-language rejection, merchant identity rejection, scroll-effect identity rejection, incompatible schema rejection and raw SQL rollback/reapplication readback. No player tables were copied and the production database was not modified. All 182 new merchant slots were checked for collisions and the existing merchant's other policies were preserved. The new source/schema requires matching binaries and coordinated deployment. Native language-shop acceptance remains pending.

The final combined source (shared-core 17d63f7d, merchant language engine and registered schema 9336) linked both world and zone successfully using GCC 12/C++20 and the normal CMake/PCH configuration in an isolated, resource-limited, network-disabled container. This compile/link evidence is separate from native gameplay acceptance and does not install the binaries.

## Ordered runes and ground auras

The original ordered rune catalogues passed 428 offline LuaJIT cases covering all catalogue prefixes, spell and discipline learning, permanent class selection, one-rune consumption from a stack, extra/wrong item return and owner checks. Ground-position checks and the actual target-ring dispatch passed 3,969 finite/range/boundary and dispatch cases. The combined world and zone source linked successfully in the isolated GCC 12/C++20 build. These are offline checks; no game session was launched or controlled.

Stage 006 adds the verified Drifting Haze III item identity using the existing rank II sibling for neutral fields. Source legacy zero level fields were not used to erase its dated level 76 requirement. No original spell levels/effects were replaced. One guide-listed SoF bard scroll remains unresolved and is recorded in the provenance. VoA Cantrip/Periapt and other expansion acquisition work remains separate and incomplete.

The expanded installer passed a complete isolated MariaDB application with all five quest/module files, scroll readback, identical no-op rerun, numeric/named quest conflict refusal, and stage 006 SQL rollback/reapplication. An injected failure immediately before database commit rolled back both database additions and newly installed quest files/directories. No player tables were copied and production content was unchanged.
