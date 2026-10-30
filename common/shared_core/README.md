# Shared RoF2 server infrastructure

Canonical ownership: `EQEmu::SharedCore`. This library has no includes from `common/housing`, fellowship, mercenaries or shrouds. It supplies authenticated actor identities, exclusive inventory session leases, exact recursive item row/codec/projection helpers, optional pending-operation quarantine and supervised process proof. DBcore reconnect storage and query-failure generation are also inherited here.

Legacy `Housing` type aliases and SQL/custom-data names remain for source and stored-byte compatibility. They do not enable housing or provision a property. Optional housing tables are inspected only to prevent unsafe writes when old unresolved operations exist; absent tables are accepted only after exact missing-table proof. Their schemas and recovery workflows remain in the owning feature.

Only `utils/sql/shared_core/001_inventory_sessions.sql` is the core schema. It is the same DDL as the existing housing 009 migration, with the same durable table names. Existing matching deployments must adopt/review their schema rather than create renamed duplicate session histories. Do not clear active leases or invent timeout/death proof. Housing's ordered installer still records its original migration identity.

Core alone compiles into `common`; it does not activate client session hooks or change character admission. Each feature owns its runtime activation and lifecycle integration. The full housing crash reconciler and property receipt recovery remain with housing; this core checks registration but does not claim that arbitrary session deaths can be automatically recovered.

No new network opcode belongs to this library. Feature mappings and structures are documented on their feature branches.
