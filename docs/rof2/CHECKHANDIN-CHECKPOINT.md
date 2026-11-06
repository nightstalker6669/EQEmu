# CheckHandin tentative item counts

Failed NPC hand-in checks must preserve the live item bucket. This change matches against a local copy, commits counts only after item and exact money requirements succeed, and gates guildmaster discipline processing on the same accepted result. Exact money equality is unchanged.

Only three source hunks in `zone/npc.cpp` are included. No quest, achievement, schema, header, CMake or private test entry changes are included.

Validation: the corresponding integrated candidate compiled and passed one bounded headless run covering 13 ordered inputs across 11 categories, including actual NPC/Client construction and destruction, inventory return and currency SQL receipts. The public shared-core baseline has not been separately compiled. Guildmaster discipline, Lua callers, task API delivery, full FinishTrade, native client, relog/logout and global teardown acceptance remain pending. The headless run does not establish economic exactly-once behavior or crash durability.

This is a source publication checkpoint. No deployment, runtime enablement or schema/content change is implied.
