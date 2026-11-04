# QA39999 serialized weakening profile — final source

This package replaces the existing temporary QA39999 two-million damage profile with a one-hundred-thousand weakening profile on the same ID. It supplies guarded helper/wrapper source, the exact single client row, full server row images and original owned-file rollback bytes. It is publication-ready source; no deployment, native application or test is performed or authorized by these files.

The existing spell adds SPA220 SkillDamageAmount to real 1HBlunt hits. Only effect_base_value12 and two display messages change. The fixed row remains self-only, ten ticks, nonfocusable, nonpersistent, and otherwise unchanged; there is no new ID, item, NPC/HP/skill edit, artificial damage or kill credit. Actual native mitigation, modifiers, misses and multiple attacks per round still apply. 100k is not an upper bound on total damage or a guarantee against overshooting.

## Gates that distinguish the profiles

- `#aidapply` accepts only the actual loaded native39999 slot11 SPA220/base2000000/base2=0/max0/formula100 profile, for `kill_objectives`. It refuses the100k profile.
- `#aidweaken` accepts only the actual loaded same profile with base100000, for `weakening`. It refuses a stale2M cache.
- Both application paths require the real character337, GM enabled, Admin at least200, level100, exact primary147495 in slot13, and native autoattack/autofire OFF. The helper also checks self target6, beneficial1, ten ticks/formula7, zero range/AE/mana/cast time, nonpersistent death behavior and blank remaining effect slots. These checks read existing native Lua_Spell methods; they are not a helper-local assertion that a cache refresh succeeded.
- `#aidremove` requires the same identity/GM/admin/level and stopped-native-attack guards, fades only39999, and verifies native absence. Cleanup does not require the weapon so an equipment change cannot strand the aid. Native receipt fields include phase/base, stopped flags and original/current buff IDs; actual runtime receipts remain private with the tester.

The helper casts only from that client onto itself. Existing/pending aid, incompatible profile, stacking conflict or uncertain application stops the operation; no original buff is removed to make room. There is no new command evaluator, public/global dispatch, automatic attack, cast-on-zone behavior or capture action. Stick/nav controls and their stopped observation remain the client's responsibility; server attack flags do not prove those controls stopped.

Use one manually gated attack round at a time while observing actual HP. Stop coarse weakening conservatively at about40% remaining. Issue attack/stick/nav OFF, verify actual native stop, remove39999 and obtain native absence BEFORE any capture, solution/item/trade action, phase transition, zoning or relog. Finish any smaller adjustment unbuffed with the retained weapon. Multiple landed hits in a round may overshoot; never leave autoattack running toward a capture threshold. Turn GM off only after guarded removal/absence, since cleanup itself requires the GM/admin guard.

## Exact published Database dependency

`database-source-binding.json` pins Database's six already-published sources at commit `09262f972a96d7e75b78928b5aeb806b57ae00ce` on `rof2-spells`. Its existing-row UPDATE and inverse lock/guard all237 fields, change exactly three fields, require one affected row and provide separate independent postcommit readback contracts. The old INSERT/DELETE helper is not a replacement and is excluded. Do not copy the six published Database files as another component or publish them again through this manifest.

The published Database contract's proposal hashes identify the historical original source input; final helper/wrapper/proposal metadata is pinned by this package's publication manifest. Both full row images and the exact three-field delta match that Database version. The original metadata preimage is retained privately for lineage; it is not an alternate deployment contract.

Source publication does not execute the UPDATE. JARVIS must pin the actual current native attack-off/aid-absent receipt, direction, exact published SQL and one-operation GO. Database owns that transaction and independent full-row readback. An uncertain result is reconciled read-only; neither retry nor automatic inverse is permitted. Restoring2M does not remove this temporary QA spell or settle its eventual cleanup debt.

## Combined window and rollback

`WINDOW-CONTRACT.md` specifies one future JARVIS-owned window with Binding846's assigned Build. Database commit/readback, server cache freshness, client row freshness and native helper receipts are distinct outcomes. Do not restart anything independently. Build owns the exact supported reload/process path; an unchanged row count or written mmap does not prove the active Zone loaded the desired profile.

The client owner replaces only the exact39999 line, whose237-field row changes only zero-based indices6,8,31. The full client-file backup and unrelated edits remain in that owner's custody; never replace the whole file with this one-line artifact. No extra client installation is created.

Rollback uses the published inverse full-row UPDATE, retained client2M row and exact original helper/wrapper bytes, under fresh attack-off/aid-absent and owned-byte preimages. Do not load the old kill-only helper against a100k row/cache. Keep the guarded helper through the profile transition, establish actual2M server/client freshness, then restore the original owned files if JARVIS requests full source rollback. Preserve other buffs, gear, property, earned progress, profiles, rules and unrelated files. No old full-file restore may overwrite another owner's later edits.

No new tests, preflights, namespace audits, database reads/writes, runtime edits or player actions were performed for this source package.
