# Shared RoF2 server infrastructure

Canonical ownership: `EQEmu::SharedCore`. This library has no includes from `common/housing`, fellowship, mercenaries or shrouds. It supplies authenticated actor identities, exclusive inventory session leases, exact recursive item row/codec/projection helpers, optional pending-operation quarantine and supervised process proof. DBcore reconnect storage and query-failure generation are also inherited here.

Legacy `Housing` type aliases and SQL/custom-data names remain for source and stored-byte compatibility. They do not enable housing or provision a property. Optional housing tables are inspected only to prevent unsafe writes when old unresolved operations exist; absent tables are accepted only after exact missing-table proof. Their schemas and recovery workflows remain in the owning feature.

The inventory/session schema remains `utils/sql/shared_core/001_inventory_sessions.sql`. It is the same DDL as the existing housing 009 migration, with the same durable table names. Existing matching deployments must adopt/review their schema rather than create renamed duplicate session histories. Do not clear active leases or invent timeout/death proof. Housing's ordered installer still records its original migration identity.

The inventory/session library compiles into `common`; its session hooks remain feature-owned. The shared task reward layer also compiles into `zone` and `world` and preserves the published task admission and durable reward behavior. Each feature owns its runtime activation and lifecycle integration. The full housing crash reconciler and property receipt recovery remain with housing; this core checks registration but does not claim that arbitrary session deaths can be automatically recovered.

The shared task reward infrastructure also owns the native reward managers and task ledgers. See [shared rewards and migration compatibility](../../docs/rof2/SHARED-REWARDS.md). Achievement progress, criteria, providers and content remain on the achievement branch.
