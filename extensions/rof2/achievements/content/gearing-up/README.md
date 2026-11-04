# Gearing Up — eight quest source candidate

Status: **disabled; not accepted for installation or native gameplay yet**.

This package reconstructs Tabazin Gearflinger’s eight repeatable Gearing Up tasks in Fortress Mechanotus. It includes 15 native delivery objectives and eight achievement component bindings for achievement 500120600. It preserves the researched player-choice selector and one-active-Gearing-Up rule; Screw Loose remains a separate task.

The existing NPC placement and item drop tables are retained. No parent item reward is invented. XP 1000 is a labelled configurable PEQ-style default, not a recovered historical amount. The original minimum level and faction reward amount are unresolved; disabled zero-valued SQL placeholders must not be advertised as verified historical policy.

## Required engine dependency

Requires shared-core commit `94e5f72768635a504c9b13feed093ddcef81d82b` or a reviewed descendant with its native delivery custody behavior. That source change must be built, deployed and verified separately. The task-only trade handler calls CheckHandin once when native-delivered quantities exist, then ReturnHandinItems once. A normal false match (such as extra coins) is distinct from a thrown error. Do not combine this handler with another item consumer, strip coins, refund guessed quantities or roll back task credit.

## Before enabling

Resolve the admission/reward configuration, verify the exact existing Tabazin callback before replacement, and register the reserved migration 9340 on the correct shared-core descendant. The supplied SQL is disabled review material, not a drop-in updater. Do not change the running database version or overwrite another callback to make it install.

The accepted native milestone must include actual item consumption/returns, task and component completion, confirmed rewards, Screw Loose coexistence and one persistence cycle for all eight quests. Current Lua checks are source-level evidence only. No native success, SQL installation or Windows acceptance is claimed here.

Existing task/achievement protocol messages are used; this candidate adds no opcode or packet structure. See the repository’s shared protocol documentation for recovered RoF2 layouts.
