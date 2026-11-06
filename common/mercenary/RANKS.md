# Mercenary rank content

The May 2013 catalogue supports **Apprentice I–V and Journeyman I–V** for tank (warrior), healer (cleric), melee damage (rogue), and caster damage (wizard) roles. There is no released Master rank to restore. The [official historical FAQ](https://www.everquest.com/news/imported-eq-enus-51873) describes five Apprentice and five Journeyman tiers; [community historical notes](https://www.bonzz.com/mercs.htm) describe Master as announced but never released.

## Supplied content gap

The optional upstream bootstrap supplies all five Apprentice tiers but only Journeyman I–II templates and stats. Higher Journeyman NPC metadata and appearance ranges exist without working hireable templates. A missing race/role combination or an Apprentice-only specialist is a separate catalogue policy, not proof of missing ranks.

`utils/sql/mercenary/005_complete_journeyman_ranks.sql` fills Journeyman III–V for each race/role combination already offering Journeyman II. It adds 1,020 stat rows (12 NPC types × levels 1–85) and 237 templates to the stock snapshot, bringing totals to 3,400 stat rows, 40 NPC types and 790 templates. Merchant links extend only catalogues already offering the matching Journeyman II. It preserves specialist, event and claim catalogue boundaries and existing rows. Apprentice content needs no rank migration.

This is a content change. Existing rank/proficiency fields, shop serialization, contracts and gameplay support the restored tiers; no opcode or wire-structure changes are required. See [PROTOCOL.md](PROTOCOL.md) for those definitions. Original-client rank labels use the existing dbstring convention, with final digit 3, 4 or 5.

## Explicit emulator tuning

Historical Journeyman III–V stat tables were not recovered. Newly generated rows use **110%, 120%, and 130% of the same class and client-level Journeyman II**, respectively. These are documented server tuning choices, not recovered Live balance.

Positive HP, mana, AC, ATK, attributes, resists, minimum/maximum melee damage, accuracy, regeneration, and spell/heal scaling are multiplied. Integer results round down and saturate at their destination column limits. Zero and negative sentinel values remain unchanged. Level, attack count/speed/delay, special abilities, running speed and `statscale` remain unchanged. Supplied higher-tier appearance/equipment ranges are retained; absent armor/weapon ranges inherit Journeyman II. Existing subtype equipment and class/proficiency stance/spell lists continue to apply.

For example, the supplied level 85 tank has Journeyman II HP 15,583; generated III/IV/V HP are 17,141 / 18,699 / 20,257. Its maximum melee damage 512 becomes 563 / 614 / 665. Above level 85, the existing rule-controlled level 86–100 content fallback applies to these new ceilings. Existing custom target rows are never rebalanced on rerun, so later tuning requires an explicit operator change.

The migration inherits current Journeyman II availability, price calculations and spell policy. It does not reconstruct historical account entitlements, faction/progression unlocks, fear/confidence curves, or separate tier-specific spell upgrades. Stronger numeric stats do not establish exact Live effectiveness or balance.

## Installation and reload

Back up the five destination content tables and review custom data. Apply after provisioning the standard catalogue, using an exclusive content-maintenance window and a MariaDB client that stops on the first error, without forced continuation or automatic reconnect. Select the database used by the existing mercenary template loader; this migration expects the mercenary tables together there. All five destination tables must use InnoDB. It creates connection-local temporary helpers, validates the four baseline roles and metadata, and commits inserts together. On any error, roll back or disconnect; do not continue toward `COMMIT` manually.

The migration inserts absent stat levels, templates, merchant links, and appearance ranges. It leaves owned contracts, buffs, wallets, rules and existing custom rows intact. It refuses conflicting target-template NPC mappings, missing or ambiguous target subtypes, and baseline dbstrings outside the stock numeric/final-digit convention. Reapplying a successful migration is a no-op.

Zones cache templates and catalogue data. A newly booted zone loads the restored ranks; already running zones need their normal controlled reload/restart before advertising them. Do not assume a rule reload refreshes catalogue maps. For an otherwise installed server this data-only update needs no C++ rebuild or opcode replacement.

Validate the actual liaison's race/role/proficiency offers in the matching RoF2 client, hire and activate a restored tier, and check server-authoritative rank and stats. Preserve custom contracts and use disposable test characters for acceptance.

## Evidence boundaries

Real MariaDB verification covers complete generated stat/armor/weapon joins, repeat-import equality, custom-row preservation, integer limits and sentinel values, rejected missing baselines/subtypes, conflicting mappings, unsupported label conventions, and nontransactional destinations, and rollback after writes. These checks establish migration behavior; native gameplay is a separate acceptance step.

Local original-client acceptance on **2026-10-30** passed all 12 restored tier/role combinations using a level 85 disposable character, Guardian Vaehan's Human catalogue and a freshly booted Plane of Knowledge instance. Each case checked the selected tier description, authoritative owned template/class/tier, an actual level 85 mercenary spawn, persisted HP/mana matching its installed stat row, and successful native dismissal. Apprentice and Journeyman I–V offers were visible. This validates the content on the deployed local build; it does not claim all races, combat balance or level 86–100 native acceptance for this update. No C++ source or protocol changes were required.

Modern Live cannot supply the old intermediate-tier catalogue: the [May 18, 2022 update](https://www.everquest.com/update-notes/eq-update-notes-5-18-2022) removed tiers, retaining Apprentice and Journeyman at the former Tier V. A current Live account's restricted offers are not evidence of May 2013 availability or historical numeric stats. Client files, credentials, captures and private test fixtures are excluded from this repository.
