# EQEmu restoration roadmap

A capability map of our additions to EQEmu and ProjectEQ for the May 10, 2013 RoF2 client. This is the first reviewed inventory, not a promise that all content through Rain of Fear is complete.

**Snapshot: November 15, 2026.** [Interactive map](docs/rof2/roadmap/index.html) · [Project overview](README.md) · [Feature source](extensions/rof2/)

The sections below expand directly on GitHub. GitHub displays the interactive HTML as source: download that file and open it in a browser for the clickable map. Hosting through GitHub Pages is a separate optional step; no hosted URL is claimed here.

```mermaid
flowchart TD
    Base["EQEmu + ProjectEQ"] --> Systems["Player systems"]
    Base --> Zones["Quests and zones"]
    Base --> Tools["Server and tools"]
    Systems --> Social["Housing / Fellowships / Mercenaries"]
    Systems --> Growth["Shrouds / Achievements / Spells / AA / Tradeskills"]
    Zones --> TSS["The Serpent’s Spine / Hero’s Journey"]
    Zones --> TBS["The Buried Sea / Secrets of Faydwer"]
    Zones --> SoD["Seeds of Destruction"]
    Zones --> RoF["Rain of Fear"]
    Tools --> Admin["EverForge / Shared foundation / Compatibility"]
```

## Reading the status

| Status | Meaning |
| --- | --- |
| Published scope/package | Available in public source or reviewed content packages; supported scope varies. This alone is not proof of activation or every gameplay variation. |
| Published + ongoing | Reviewed work exists, with further families or batches unfinished. |
| Native accepted · local | The recorded local gameplay scope passed; it is not yet a published download. |
| Source package · acceptance pending | Implementation is available for review; activation or gameplay acceptance remains pending. |

## Player systems

<details>
<summary>Housing — Published scope</summary>

Personal and guild homes built around the native RoF2 housing interface.

- **Neighborhoods & plots:** Sunrise Hills neighborhoods, plot purchase, upkeep and property access.
- **Homes & guild halls:** Place houses and halls, enter their interiors, and route guild members through the Guild Lobby.
- **Decoration & layouts:** Move, rotate and scale furnishings; wall and ceiling placement; save and restore layouts.
- **Storage & services:** Closets, vaults, sheds and moving crates; parcel returns; supported bankers, vendors and travel services.

**Remaining / qualified scope:** Supported item identity and contents are preserved across moves. This is a substantial native housing implementation, not a claim of every Live housing feature.

[Source and scope notes](extensions/rof2/housing/)

</details>

<details>
<summary>Fellowships — Published scope</summary>

Small-group social features, camps and shared experience.

- **Membership & chat:** Up to 12 members, invitations, leadership, MOTD, native window and cross-zone fellowship chat.
- **Campsites:** Free and paid camps, kit/lumber costs, expiry and nearby combat or utility effects.
- **Travel & access:** Insignia travel with instance admission checks; fellowship-aware housing permissions.
- **Experience sharing:** Normal and AA experience sharing, offline vitality banks, redemption and native gauges.

**Remaining / qualified scope:** Broad lifecycle checks were exercised with three native clients. Every individual camp recipe and exact Live balance are not claimed.

[Source and scope notes](extensions/rof2/fellowship/)

</details>

<details>
<summary>Mercenaries — Published scope</summary>

Native hiring and management with four roles and progression through level 100.

- **Four roles:** Tank, healer, melee damage and caster damage, with level 86–100 stat and spell support.
- **Management:** Hire, suspend, revive, select stances and manage persistent contracts in the native interface.
- **Combat & upkeep:** Role-aware assistance, healer behavior, coin upkeep and Bayle Mark fallback.
- **Ownership & progression:** Configurable multiple owned contracts with one active; earned Journeyman II acquisition in the accepted Oceangreen slice.

**Remaining / qualified scope:** Some scaling is reconstructed. Marketplace slot/rename commerce, later mercenary AAs and player-equipped mercenary gear are not implemented. Earned J3 progression has local native acceptance for one ordinary level-100 healer hire; its matched source, database and server download are published. See Kithicor in Quests & zones. Field of Scale adds recorded Journeyman IV Human Healer offer/one-hire acceptance; final consumer QA retained developer access with GM mode off, not ordinary-status-0 positive certification. Other race/role coverage is not inferred.

[Source and scope notes](extensions/rof2/mercenaries/)

</details>

<details>
<summary>Spirit Shrouds — Published scope</summary>

Native transformation, original-character restoration and earned shroud progression.

- **Selection & transformation:** Native selection and preview, original-character escrow and restoration.
- **Enabled forms:** Eight starting forms, in level steps from 5 through 70: 112 enabled combinations.
- **Abilities & rewards:** Monster Point purchases, 47 paid AA groups / 329 ranks, and earned-item retrieval.
- **Persistent accounting:** Original character XP/AA accounting and recovery for corpse and transformation state.

**Remaining / qualified scope:** Later branches remain disabled or incomplete. Enabled catalogue coverage does not establish gameplay acceptance of every form and level.

[Source and scope notes](extensions/rof2/spirit-shrouds/)

</details>

<details>
<summary>Achievements — Published scope</summary>

Native achievement tracking connected to restored quest routes.

- **Native display:** Definitions, categories, progress, comparisons and completion notifications.
- **Durable progress:** Persistent achievement and task state, with event hooks for quest authors.
- **Reward choices:** Persistent reward options, permanent-class filtering and common reward claims.
- **Content packages:** Scoped Hero’s Journey and Rain of Fear quest groups; select Quests & zones for the content breakdown.

**Remaining / qualified scope:** The researched achievement inventory is larger than the playable catalogue. Entire expansion completion is not claimed.

[Source and scope notes](extensions/rof2/achievements/)

</details>

<details>
<summary>Spells &amp; acquisition — Published scope</summary>

Published restorations affect 5,327 existing player-spell IDs across 5,629 class memberships. No new spell definitions are claimed.

- **Class and level catalogue:** All indexed names and ranks, sorted by retained native learning level, with links to the source-qualified routes.
- **Acquisition and scrolls:** Merchant stock, faction/language gates, original exchanges and restored scroll items; a scroll-only restoration does not establish a working route.
- **Support content:** Summoned items and optional estimated pet/aura support, separated from ordinary baseline spell definitions.
- **Earned Void access:** 77 existing stock IDs remain distinct from earned B8 + C14 + D18 =40 cumulative. D access and one175-Chronobine purchase/learn passed with developer-access QA qualifications; E–G ordinary progression remains held.

**Remaining / qualified scope:** Source publication is separate from activation and native proof. Categories overlap and are not additive; shared-class display rows are not new spell IDs. Two House of Thule exchange routes remain on hold. Optional support is estimated, unchanged baseline spells and NPC/helper references are excluded, and class levels are retained metadata rather than a new historical audit.

[Restorations by class and learning level](docs/rof2/roadmap/spells/README.md) · [Portable searchable catalogue](docs/rof2/roadmap/spells.html)

| Class | Class spell IDs |
| --- | ---: |
| [Bard](docs/rof2/roadmap/spells/bard.md) | 324 |
| [Beastlord](docs/rof2/roadmap/spells/beastlord.md) | 338 |
| [Berserker](docs/rof2/roadmap/spells/berserker.md) | 173 |
| [Cleric](docs/rof2/roadmap/spells/cleric.md) | 516 |
| [Druid](docs/rof2/roadmap/spells/druid.md) | 494 |
| [Enchanter](docs/rof2/roadmap/spells/enchanter.md) | 510 |
| [Magician](docs/rof2/roadmap/spells/magician.md) | 434 |
| [Monk](docs/rof2/roadmap/spells/monk.md) | 152 |
| [Necromancer](docs/rof2/roadmap/spells/necromancer.md) | 443 |
| [Paladin](docs/rof2/roadmap/spells/paladin.md) | 379 |
| [Ranger](docs/rof2/roadmap/spells/ranger.md) | 370 |
| [Rogue](docs/rof2/roadmap/spells/rogue.md) | 200 |
| [Shadow Knight](docs/rof2/roadmap/spells/shadow-knight.md) | 330 |
| [Shaman](docs/rof2/roadmap/spells/shaman.md) | 391 |
| [Warrior](docs/rof2/roadmap/spells/warrior.md) | 145 |
| [Wizard](docs/rof2/roadmap/spells/wizard.md) | 430 |

[Source and scope notes](extensions/rof2/spells/)

</details>

<details>
<summary>Alternate Abilities — Published named changes; partial coverage</summary>

75 changed ability families and 198 distinct rank IDs: 125 added/restored plus 73 existing ranks affected. Shared ranks repeat in 1,804 class entries.

- **Class and rank catalogue:** Sixteen readable class lists, sorted by native required level, name and rank; acquisition minimum is a separate requirement.
- **Restorations and corrections:** Named rank additions, eligibility fixes, effect/core-bonus corrections, book acquisition, entitlement source and bot-duration work; categories overlap.
- **Race and acquisition gates:** Drakkin breaths remain race/heritage-specific; Gnome tinkering and Enchanter books retain their prerequisites and acquisition rules.
- **Held and reference scope:** Hero’s Barracks is an unchanged-definition finite-H reference; Pack Hunt rank 8 is held. Unresolved incidental consumers and disabled drafts are excluded from named totals.

**Remaining / qualified scope:** Finite published-source named-change inventory, not all-AAs-complete proof. No rank is marked native-gameplay-proven in this packet. Source publication, applied data, install/cache receipts and serving activation remain distinct. Preserve race/heritage, grant/book requirements, unknown metadata and approved numerical reconstruction limits.

[AA changes by class](docs/rof2/roadmap/aa/README.md) · [Portable searchable catalogue](docs/rof2/roadmap/aa.html)

| Class | Changed families | Rank entries |
| --- | ---: | ---: |
| [Bard](docs/rof2/roadmap/aa/bard.md) | 23 | 107 |
| [Beastlord](docs/rof2/roadmap/aa/beastlord.md) | 25 | 113 |
| [Berserker](docs/rof2/roadmap/aa/berserker.md) | 16 | 100 |
| [Cleric](docs/rof2/roadmap/aa/cleric.md) | 25 | 115 |
| [Druid](docs/rof2/roadmap/aa/druid.md) | 24 | 112 |
| [Enchanter](docs/rof2/roadmap/aa/enchanter.md) | 39 | 132 |
| [Magician](docs/rof2/roadmap/aa/magician.md) | 24 | 110 |
| [Monk](docs/rof2/roadmap/aa/monk.md) | 17 | 113 |
| [Necromancer](docs/rof2/roadmap/aa/necromancer.md) | 22 | 108 |
| [Paladin](docs/rof2/roadmap/aa/paladin.md) | 24 | 123 |
| [Ranger](docs/rof2/roadmap/aa/ranger.md) | 37 | 125 |
| [Rogue](docs/rof2/roadmap/aa/rogue.md) | 16 | 102 |
| [Shadowknight](docs/rof2/roadmap/aa/shadowknight.md) | 22 | 121 |
| [Shaman](docs/rof2/roadmap/aa/shaman.md) | 24 | 112 |
| [Warrior](docs/rof2/roadmap/aa/warrior.md) | 17 | 103 |
| [Wizard](docs/rof2/roadmap/aa/wizard.md) | 22 | 108 |

[Source and scope notes](extensions/rof2/aa/)

</details>

<details>
<summary>Tradeskills — Published + ongoing</summary>

11,662 distinct added recipe IDs, grouped by craft; existing recipe fixes and access changes are listed separately.

- **Added recipe catalogue:** Alphabetical craft lists include physical recipe variants, names, IDs, families and accepted trivial values.
- **Existing recipe changes:** Corrections to ingredients, yields, containers, recipe availability and access are separate from additions; overlapping counts are not summed.
- **Special crafting:** Charged items, eight reviewed recharges and quest crafting consumer support; recharge gameplay acceptance is not established.
- **Scope limits:** Three exact-trivial variants remain unsupported; approved local scalar choices and source-unconfirmed stew remain qualified.

**Remaining / qualified scope:** Finite accepted/published owner inventory, not a complete through-RoF audit. Counts are distinct added IDs, not unique output names or the full PEQ database. Fletcher Ceramic Clay remains unclassified stored skill 62. Application/publication evidence is not native gameplay proof.

[Added recipes by craft](docs/rof2/roadmap/tradeskills/README.md) · [Portable searchable catalogue](docs/rof2/roadmap/tradeskills.html)

| Tradeskill | Added recipe IDs |
| --- | ---: |
| [Alchemy](docs/rof2/roadmap/tradeskills/alchemy.md) | 788 |
| [Baking](docs/rof2/roadmap/tradeskills/baking.md) | 1,300 |
| [Blacksmithing](docs/rof2/roadmap/tradeskills/blacksmithing.md) | 2,091 |
| [Brewing](docs/rof2/roadmap/tradeskills/brewing.md) | 143 |
| [Fishing](docs/rof2/roadmap/tradeskills/fishing.md) | 26 |
| [Fletching](docs/rof2/roadmap/tradeskills/fletching.md) | 637 |
| [Jewelry Making](docs/rof2/roadmap/tradeskills/jewelry-making.md) | 1,393 |
| [Make Poison](docs/rof2/roadmap/tradeskills/make-poison.md) | 15 |
| [No Skill / quest and container combines](docs/rof2/roadmap/tradeskills/no-skill-quest-and-container-combines.md) | 102 |
| [Pottery](docs/rof2/roadmap/tradeskills/pottery.md) | 1,846 |
| [Research](docs/rof2/roadmap/tradeskills/research.md) | 1,209 |
| [Tailoring](docs/rof2/roadmap/tradeskills/tailoring.md) | 1,589 |
| [Tinkering](docs/rof2/roadmap/tradeskills/tinkering.md) | 522 |
| [Unclassified stored skill 62](docs/rof2/roadmap/tradeskills/unclassified-stored-skill-62.md) | 1 |

[Source and scope notes](extensions/rof2/tradeskills/)

</details>

## Quests & zones

[Browse the alphabetical quest-route catalogue](docs/rof2/roadmap/quests/README.md) · [Portable searchable catalogue](docs/rof2/roadmap/quests.html)

**356 documented named task routes, five descriptive scripted exchange routes and twelve held/source-only appendix entries.** These counts distinguish source work, accepted tasks and held content; they are not 373 completed or newly added quests. Multi-zone crosslinks do not increase global totals. The Achievements subset retains its 108 exact saved receipt / eleven coherent-source summary / 172 older-overlay unresolved-receipt tiers. Existing zone-package scope and native limitations below remain unchanged.

- **Prophecy of Ro group progression through Theater of Blood.** RoF2 native core acceptance includes scoped positioning and combat assistance. Earned chime and AA entry passed. The Library v10 exit, standalone level gates and automatic Sverag rotation remain unverified. Deathknell and Tunare are separate. [Zone and spell acquisition scope](docs/rof2/roadmap/PROPHECY-OF-RO.md).

### The Serpent’s Spine / Hero’s Journey

<details>
<summary>Crescent Reach — Published package</summary>

Scoped starter and achievement quest restoration.

- **Main routes:** Council of Six, Making a Name for Yourself and Becoming a Hero.
- **Supporting content:** Starter definitions, prerequisites, recipes, objects, drops and merchant supplies.
- **Baker Shivra:** Both Baker routes, native hand-ins, rewards and persistent achievement checks.

**Remaining / qualified scope:** Graduate of Crescent Reach is not fully native-validated; unrecovered rewards are not invented.

[Source and scope notes](extensions/rof2/achievements/content/crescent/)

</details>

<details>
<summary>Blightfire Moors — Published packages</summary>

Several restored quest families within one zone.

- **General quest routes:** Core Moors package and its achievement groups.
- **Bixies & Makesn:** Bixie-related routes and supporting quest objectives.
- **Spirits & combat:** Battling the Blightfire Spirits and combat-focused quest routes.
- **Welcome & Champion:** Welcome to the Moors and Champion routes, with parent reward dependencies.

**Remaining / qualified scope:** Package families overlap zone content; their counts are not added together as a unique zone-wide completion total.

[Source and scope notes](extensions/rof2/achievements/content/)

</details>

<details>
<summary>Stone Hive — Published packages</summary>

Quest and achievement routes, including A Short Sharp Sting.

- **Core package:** 16 quest routes and four achievement groups in the scoped package.
- **A Short Sharp Sting:** Journal/container work, recipes, faction and copied encounters.
- **Rewards & objectives:** Quest-object interactions, component progression and restored reward paths.

**Remaining / qualified scope:** Package inclusion is separate from native Windows acceptance. Exact scope and provenance remain in each package.

[Source and scope notes](extensions/rof2/achievements/content/stonehive/)

</details>

<details>
<summary>Goru’kar Mesa — Published packages</summary>

Core routes plus additional Mesa quest families and encounters.

- **Core quest routes:** 19 routes with native completion recorded; the twentieth capture quest remains disabled.
- **Additional families:** Armor, paid gifts, centaur and Bring Friends-related content.
- **Scripted encounters:** Kathryn’s defense, corpse-note work and Simu’s escort.
- **Supporting crafting:** Additional task activities, quest recipes and historical achievement definitions.

**Remaining / qualified scope:** Missing head rewards, Berserker Pranks and the disabled capture route remain distinct gaps.

[Source and scope notes](extensions/rof2/achievements/content/mesa/)

</details>

<details>
<summary>Blackfeather Roost — Published package</summary>

Restored quest routes, jewelry reward paths and nest encounters.

- **Quest coverage:** 15 scoped quests and their activities and achievement definitions.
- **Rewards:** Three parent jewelry reward sets.
- **Encounters:** Nest combat and lifecycle scripting.

**Remaining / qualified scope:** Two armor reward-choice sets remain unresolved.

[Source and scope notes](extensions/rof2/achievements/content/roost/)

</details>

<details>
<summary>The Steppes — Published package</summary>

Quest routes and shared crypt content.

- **Quest & rewards:** 13 scoped routes, achievement definitions and four reward sets.
- **Shared crypt:** Three-player native checks, shared rejoin and completed-member reconnect behavior.

**Remaining / qualified scope:** Late joining and pickup behavior retain documented limitations.

[Source and scope notes](extensions/rof2/achievements/content/steppes/)

</details>

<details>
<summary>Sunderock Springs — Published package</summary>

Restored quest routes with item reward paths.

- **Quest routes:** 15 scoped routes and 56 objectives.
- **Rewards:** Four parent-item reward paths and supporting quest state.

**Remaining / qualified scope:** Numeric faction rewards and Mines raid-access work remain unfinished.

[Source and scope notes](extensions/rof2/achievements/content/sunderock/)

</details>

<details>
<summary>Vergalid Mines — Published package</summary>

Quest objectives, restored actors and persistent key/object interactions.

- **Quest routes:** 17 scoped quests, four parent reward paths and seven restored actor types.
- **Interactive objects:** Chest and bait behavior, charged-shard statue and shared orb.
- **Persistence:** Key state and shared quest progress handling.

**Remaining / qualified scope:** Multiplayer and Windows gameplay acceptance retain package-specific limits.

[Source and scope notes](extensions/rof2/achievements/content/vergalid/)

</details>

### The Buried Sea / Secrets of Faydwer

<details>
<summary>The Buried Sea — Published scope</summary>

Island quest progression, the skeletal-hand collection and a real group mission.

- **Island routes:** 39 scoped island quests and seven armor reward sets.
- **Jonas Dagmire’s hand:** All 27 bones and seven native compound recipes.
- **Mer’s mission:** Real three-member mission, member rewards and Halon stock.
- **Travel:** Ferries and rowboats, with native completion and login persistence checks.

**Remaining / qualified scope:** This is the reviewed island/mission slice, not a claim that the whole expansion is restored.

[Source and scope notes](extensions/rof2/achievements/README.md)

</details>

<details>
<summary>Dragonscale Hills — Published package</summary>

Quest routes spanning three achievement families.

- **Quest families:** Strike Force, Challenges and Ladies of the Light.
- **Quest objects & scenes:** Farm gears, banner, crystal canvas and rescue interactions.
- **Acceptance:** Native completion of all three scoped achievement families and relog persistence recorded.

**Remaining / qualified scope:** Scope covers the 15-route package, not every quest or encounter in Dragonscale Hills.

[Source and scope notes](extensions/rof2/achievements/content/dragonscale/)

</details>

### Seeds of Destruction

<details>
<summary>Oceangreen → Void B / J2 — Published · native accepted</summary>

The newly accepted ten-task village and Blackburrow progression slice.

- **Ten tasks:** Six personal tasks and four shared missions, completed on the matched local server.
- **Shared missions:** Stop the Contamination, A Dread Challenger, Freedom! and Beat the Blackburrow Boss; once-only rewards for two real recipients.
- **Earned progression:** Void B admission, real scroll purchase and scribing, and ordinary level-100 Journeyman II healer hiring.
- **Travel & persistence:** Hills/Blackburrow travel and mission returns; history, currency, rewards and mercenary state checked through fresh logins.

**Remaining / qualified scope:** Upper-ward physical blocking remains LIMITED. Encounter numbers are local reconstruction. Linux native acceptance does not establish Windows gameplay acceptance.

[Source and scope notes](docs/rof2/OCEANGREEN.md)

</details>

<details>
<summary>Kithicor → Void C / J3 — Published batch · native scope qualified</summary>

Matched source, database and r25 server downloads are published. Recorded native gameplay acceptance remains the matched local Linux scope.

- **Six tasks & shared rewards:** Charges of Treason, Vengeance and Deliverance, Destroy the Impostor, Rain of Ruin, Safe Passage and Bitter Victuals. Both physical escort stages, refugees and rewards passed; 31 reward components were paid, with two real shared-mission recipients.
- **Earned Void C:** Original door progression into Void C and 22 cumulative vendor rows checked through the native client.
- **Scroll & Journeyman III:** One 165-Chronobine Mindfreeze scroll purchase and spell scribing, then one ordinary level-100 Journeyman III healer hire. Existing contracts, spells and items were preserved.
- **Published matched batch:** Source 5a9d8a0, the peq-rof2-2026-11-15-kithicor database and server release rof2-ci-r25-a1-5a9d8a0b3c8b are public. Windows/Linux CI passed; native gameplay coverage retains its existing limits.

**Remaining / qualified scope:** Native acceptance used the matched local Linux cohort and r8 physical escort revision. The r11 escort-homing fix has focused mock and exact staging evidence only, pending a future fresh load; a full r11 escort walk and every guard route are not proved. Earlier 615/616 guard behavior causes and the peer NPC ready path remain unproved. The owned AA exit and main NPC ready path passed. Numeric reconstruction is user-approved, not exact historical parity. Windows native gameplay is not established by CI. Oceangreen geometry limits continue to apply.

[Source and scope notes](https://github.com/nightstalker6669/EQEmu/blob/5a9d8a0b3c8bfc7a2c5728cc9c006d640e2d33d6/docs/rof2/KITHICOR.md)

</details>

<details>
<summary>Field of Scale → Void D / J4 — Native accepted · QA qualified</summary>

Nineteen authored tasks completed and their recipient rewards paid. The five ordered group missions earn Void D and Journeyman IV eligibility; source publication and matching server-package availability are separate.

- **Nineteen stable tasks:** Shared and personal routes, including three distinct Sarnak Recovery note branches, with native completion and original recipient rewards.
- **Earned Void D:** 18 existing D stock IDs at175Chronobines; B8 + C14 + D18 =40 cumulative of77 indexed stock IDs. One item119438 purchase and existing spell19688 learn passed.
- **Journeyman IV:** One Human Healer TierIV offer and hire passed while existing owned contracts/player-earned state were retained. This is not an all-role/race mercenary acceptance claim.
- **Original progression:** Requester-side history and the five-mission sequence, authored owned entrances, native entry credit and once-only archival/reward delivery retain their documented scope.

**Remaining / qualified scope:** Final positive consumer QA retained developer-level access with GM mode off; it is not ordinary-status-0 positive access certification. One existing Wizard scroll purchase/learn and one Human Healer Journeyman IV offer/hire passed; this does not certify all spells/classes/roles. QA used maximum-level geared characters, a test weapon for routine kills and approved position assistance. Real spell cycles established the 60/30/20-percent mechanics; no forced HP/race/cast/RNG/task counters or completion-history edits established acceptance. Assisted movement does not prove full ordinary travel, collision or path coverage. Food used four natural carcasses and sixteen seeded QA ingredients. Some Sarnak actors/note sources were approved NPC-type fixtures with real loot RNG and actual combat; Burynai note provenance was natural, while the final Kurn note used a post-cache-refresh fixture. These are not wholly natural drop/respawn proofs. Required item definitions must be present in the deployed item cache before ordinary loot testing. Approved local reward floors/caps, population-decay policy, XP/faction bases, NPC balance, bounds and placement geometry are reconstructions, not authenticated original Live values or coordinates. Existing Oceangreen geometry, Kithicor r8/r11 and other held qualifications remain unchanged.

[Accepted Field of Scale guide](docs/rof2/FIELD-OF-SCALE.md) · [Quest list by zone](docs/rof2/roadmap/quests/field-of-scale.md)

</details>

### Rain of Fear

<details>
<summary>Shard’s Landing — Published package</summary>

Partisan progression and portal prerequisites.

- **Partisan arc:** Nine scoped tasks, including portal prerequisite content.
- **Achievement links:** All eight configured Partisan components checked natively.
- **Group behavior:** Additional multiplayer checks retained with the package evidence.

**Remaining / qualified scope:** This package does not establish completion of every Shard’s Landing quest or encounter.

[Source and scope notes](extensions/rof2/achievements/content/shards-landing/)

</details>

<details>
<summary>Valley of King Xorbb — Source package · acceptance pending</summary>

An inactive source package containing partisan and mercenary quest components.

- **Quest package:** 14 quest definitions and 120 objective source paths.
- **Actors & scenes:** NPC profiles, caster bindings and owned scenes for Lurr, Grolak and Gronk.
- **Objects & crafting:** Garden pickups, colored crystal interactions and the Hollowflesh coffin recipe.
- **Shared support:** Native delivery, run persistence and fixed reward definitions.

**Remaining / qualified scope:** The package README records inactive content and pending native acceptance. Do not equate published source with installed or playable quest completion; this area needs receipt reconciliation.

[Source and scope notes](extensions/rof2/achievements/content/xorbb/)

</details>

## Server & tools

<details>
<summary>EverForge — Published · 0.3.0</summary>

A packaged server stack with our administration tools and persistent installations.

- **Server stack:** Published RoF2 server, setup and matched database contracts in Docker.
- **Administration:** Source-built Spire and PEQ Editor forks, real World administration and phpMyAdmin.
- **Access utilities:** SSH/SFTP and FTPS, with documented ports and network setup.
- **Backup & upgrades:** Complete private backup/restore, retained data volumes and deliberate release selection.

**Remaining / qualified scope:** Authorized original resources are supplied separately. Existing installations retain their data; ordinary restart is not a database reseed or server upgrade.

[Source and scope notes](docs/rof2/EVERFORGE.md)

</details>

<details>
<summary>Shared feature foundation — Published scope</summary>

Reusable server improvements connecting the restored systems.

- **Reward & task delivery:** Persistent reward choices, class eligibility, task credit and shared rewards.
- **Inventory custody:** Item identity, contents and delivery handling used by housing, quests and exchanges.
- **Native world interactions:** Owned click objects, transient doors, quest pickup identity and transport passenger state.
- **Quest authoring:** Supporting hooks and APIs for the restored content packages.

**Remaining / qualified scope:** Infrastructure support does not prove that every dependent scene or quest has passed native acceptance.

[Source and scope notes](extensions/rof2/shared-core/)

</details>

<details>
<summary>Client stability & releases — Published scope</summary>

Targeted compatibility repairs and matched release packaging.

- **Character selection:** Published RoF2 character-selection crash repair.
- **Interrupt messages:** Measured repair to the interrupt-message packet writer, included in the accepted Oceangreen source.
- **Release delivery:** Linux and Windows build/package workflows, server downloads and matching database references.

**Remaining / qualified scope:** Compilation, publication and native gameplay acceptance are separate evidence. A bounded repair is not a guarantee that all crashes or CI failures are resolved.

[Source and scope notes](docs/rof2/OCEANGREEN.md)

</details>

## Next roadmap refinements

- Reconcile the remaining source packages against accepted installation and release receipts.
- Keep the finite AA class catalogue aligned with accepted owner handoffs; wider AA coverage remains incomplete.
- Keep the craft-sorted recipe catalogue and class/level spell index aligned with accepted owner handoffs.
- Add individual quest names and acceptance evidence beneath each zone.
- Keep native acceptance, source publication and released downloads separate; add dates only when an actual milestone exists.

## Provenance and upkeep

The published overview was checked at [`99c1162`](https://github.com/nightstalker6669/EQEmu/tree/99c11623c2cf5ed15b0f0830a3aecea28528fc21). Content-package notes have different validation histories. Kithicor source is published at [5a9d8a0](https://github.com/nightstalker6669/EQEmu/commit/5a9d8a0b3c8bfc7a2c5728cc9c006d640e2d33d6), with the [matching database](https://github.com/nightstalker6669/EQEmu-Database/releases/tag/peq-rof2-2026-11-15-kithicor) and [r25 server release](https://github.com/nightstalker6669/EQEmu/releases/tag/rof2-ci-r25-a1-5a9d8a0b3c8b). Local Linux native acceptance retains the r8 escort / r11 focused-mock distinction, unresolved guard and peer NPC ready qualifications, approved local reconstruction and continuing Oceangreen geometry limits. CI and publication do not widen gameplay acceptance.

Original or uncertain encounter values that were reconstructed locally are not presented as authenticated historical Live values. Original EQEmu and ProjectEQ work retains its community attribution.

Roadmap Chat maintains this inventory using accepted-scope handoffs. Update this document and the interactive map together when publication or acceptance changes. Do not attach private operational receipts, credentials, character records or proprietary client assets.

[Back to project overview](README.md)

