# Verification scope

The deployed database additions were read back and checked against their plans. Kerkaz's 160 class/tier projections matched the complete grouped reward catalogue; all reward items resolved to existing spells with the expected class level and rank. Lua 5.1 syntax and a hail/keyword selector harness passed. The script uses the native task delivery/reward path and has no scripted reward summoning or trade handler.

The public installer was exercised against an isolated MariaDB schema populated only with required baseline game content. Planning, full overlay application, identical no-op rerun and refusal of a conflicting item row passed. The same application passed without housing-specific item columns. No account, character, player inventory or housing state tables were copied into that exercise. Public files omit the private fixtures, connection files and raw test output.

A read-only installer plan against the installed content reported no new rows or vendor changes. Publication checks include Python syntax, payload/SQL consistency, Lua syntax, Git whitespace checks and an allowlisted spell-only file inventory.

These checks do not establish native in-game turn-in acceptance, exact Live pet/aura balance, full spell engine correctness, or native Windows/Akk-stack execution. A dedicated native mote test and coordinated cache activation remain pending. Publishing this branch does not reload or restart a server.

## Shared-core integration

This branch inherits the published core revision `fc88a0ba79a05e437e7b61af4e38ead32131de0f`. The achievements maintainer reported successful world/zone linking and 386 wire-compatibility cases/task contracts for that exact core tree. The spell integration retains its source, protocol and migration bytes unchanged; only spell payloads and branch documentation are added. That inherited validation is separate from native gameplay acceptance of Kerkaz, which remains pending. The shared-core extraction was performed and published by the achievements maintainer, not duplicated in the spell layer.
