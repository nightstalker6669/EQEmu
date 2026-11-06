# Architecture

## Layers and dependencies

`common/fellowship/service.*` owns membership, invitations, leadership, MOTD and camp persistence. `vitality.*` owns character banks, allocation, redemption and cap admission. `journal.*` persists the final character progression and bounded audit receipt within the bank transaction. `rof2_wire.h` is an explicit little-endian byte codec, not a compiler-packed client struct.

`campsite_catalogue.*` and its two compiled includes define597 recipes, effects, native buff levels and template signatures. `campsite_materials.*` verifies durable material rows and decrements them under the camp transaction.

Zone code authenticates native actions, resolves actual nearby target entities and membership, checks three-member camp proximity, installs scoped buffs/appearance, refreshes native UI, and performs insignia movement. The zone campfire manager creates an untargetable race567 visual, polls current camps/membership and reconciles effects. Client processing refreshes fellowship state every five seconds; explicit invalidations accelerate updates.

World authenticates cross-zone chat against its current client list, loads authoritative membership and routes one delivery per eligible recipient. Receiving zones recheck fellowship and sender before displaying channel21. World forwards zero-payload refresh notifications; it never accepts roster bytes as membership authority.

## Why the housing dependency exists

Paid campsite kits/lumber are normal inventory. The existing housing infrastructure supplies authenticated inventory admission leases, persisted raw-row encoding, runtime-to-persisted identity projection, and guards against conflicting custody/coin/house operations. Reusing these prevents a second weaker item mutation path. This release therefore requires the exact published housing baseline; those existing components are not duplicated in this repo.

The included `common/housing/fellowship_membership.h` is a separate permission integration. It loads current actor and personal-owner memberships inside the caller's transaction under the fellowship guard. Four housing services and native estate metadata gain fellowship awareness. Guild plots never borrow their leader's fellowship. Membership changes refresh catalogues/scenes across zones. The base housing access policy still decides named-group precedence, capabilities, rent, lease and ownership restrictions.

A future standalone edition could extract the inventory infrastructure to a common dependency and make property access optional. That extraction has not been implemented or validated in this release; this package intentionally publishes the tested dependency chain.

## Integration points

| File or component | Integration |
|---|---|
| Common/zone CMake | Registers production fellowship sources |
| Opcode list + RoF2 conf | Adds five canonical names; maps all six native messages |
| Client packet/process | Connected handlers and periodic refresh |
| Client channel21 | `/fsay` goes to authenticated world routing |
| Zone process/destructor | Camp polling, effect reconciliation and owned visual cleanup |
| Spell effects | Insignia calling and focus-source merge |
| Character XP path | Combat-only allocation/finalizer after Lua overrides |
| Client/zone save paths | Block independent saving during XP finalization or failed sessions |
| Buff save/load | Exclude scoped campsite leases, ignore legacy marked rows |
| Character deletion | Remove membership/invitations/banks; promote earliest surviving member |
| Housing access/scene/escrow | Current guarded fellowship authorization and display refresh |
| DBcore | Stable externally allocated MYSQL storage across owner reconnect for borrowed handles |

No general petition, mercenary or unrelated housing changes are in the fellowship patch. The required DBcore fix is included because reconnect testing exposed an existing borrowed-pointer lifetime defect; retaining a freed handle would invalidate transaction safety.
