# Veil of Alaris composite achievement links

**Source candidate only — not installed or enabled.** This change supplies 15 explicit links that exact-name inference misses in Veil of Alaris Explorer, Champion of Alaris, and Paragon of Alaris.

For example, the parent label “Windsong Traveler” refers to the separately named achievement “The Windsong Sanctuary Traveler.” The candidate binds the reviewed IDs directly instead of guessing from similar names.

## Included

- The importer patch and its complete proposed source file.
- All 15 parent/component/child identities and dated 2012 source references in [period-dependencies.json](provenance/period-dependencies.json).
- File hashes in [payload-manifest.json](payload-manifest.json).

The existing `AchievementComplete` event (11), Boolean progress (3), and required-component behavior (0) are reused. There are no new opcodes or packet structures in this change.

## Safety and readiness

Both parent and child must already be selected by the importer's historical/readiness policy. This change never adds achievements to that selection. Parent names, child names, component IDs, types, sequences, labels, and required count are checked against the reviewed identities. Missing, duplicate, ambiguous, or changed identities fail closed. Unreviewed components still use the existing exact-name behavior; there is no general fuzzy matching. Direct event criteria retain precedence.

The patch does not change the historical manifest, database, updater, installer, rewards, quest scripts, or runtime configuration. The affected definitions remain disabled. Do not install it as a standalone achievement unlock: each child's quest content, historical component shape, rewards and native persistence must be accepted before activation.

## Validation and remaining work

Focused offline checks cover all 15 links, missing or ambiguous identities, shape changes, excluded parents/children, direct-policy precedence and unchanged ordinary name matching. These are source checks, not in-game completion evidence. No test scripts or private player records are included.

The Beasts' Domain mercenary reward is reported by a period source as a choice of Noble arms ornaments. Exact item identities still require recovery; this candidate grants no reward.

Apply `importer.patch` only to its exact reviewed importer preimage shown in `package.json`, then review the generated criteria alongside the existing readiness policy before any separate deployment. This package has no automatic installer. Repository licensing is unchanged.
