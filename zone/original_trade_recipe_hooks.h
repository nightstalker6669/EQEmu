#pragma once
#include "../common/sod_weighted_delivery_context.h"
#include "../common/shared_core/inventory_session_service.h"
#include <optional>
#include <string>
class Client;
class NPC;
class ZoneDatabase;

// Finite semantic extension for the single shared original-accept writer.
namespace KithicorPoisonTrade {class Writer;}
namespace OriginalTradeWriter {
namespace Weighted=EQEmu::SharedCore::WeightedPersonalDelivery;
namespace Trade=EQEmu::SharedCore::OriginalTradeRecipe;
using Lease=EQEmu::SharedCore::InventorySessions::Lease;
using NpcIdentity=SodWeightedDeliveryContext::Npc;
using Snapshot=SodWeightedDeliveryContext::Context;
// The physical writer retains the complete immutable inventory-tree preimage,
// success/abort return blobs, coin refund and lease/request identity. Semantic
// persistence references that SAME request; it stores no competing item blobs.
struct Intent {
 std::string request_key;
 Snapshot snapshot;
};
// The legacy-named KithicorPoisonTrade::Writer remains the ONE physical writer.
class CommittedAndSettled {
 friend class KithicorPoisonTrade::Writer;
 explicit CommittedAndSettled(const Intent& intent):intent_(intent){}
 const Intent& intent_;
public:
 CommittedAndSettled(const CommittedAndSettled&)=delete;
 CommittedAndSettled& operator=(const CommittedAndSettled&)=delete;
 const Intent& Get() const{return intent_;}
};
class ConfirmedFullReturn {
 friend class KithicorPoisonTrade::Writer;
 explicit ConfirmedFullReturn(const Intent& intent):intent_(intent){}
 const Intent& intent_;
public:
 ConfirmedFullReturn(const ConfirmedFullReturn&)=delete;
 ConfirmedFullReturn& operator=(const ConfirmedFullReturn&)=delete;
 const Intent& Get() const{return intent_;}
};
enum class Ack { Settled, ReconciliationRequired };
class SemanticRecipe {
public:
 virtual ~SemanticRecipe()=default;
 // Original accept only. Spells validates current PERSONAL native slot and
 // pointer/acceptedtime/type2/was_rewarded0 under the SAME lease/connection.
 // It refuses missing policy or a durable-vs-memory count discrepancy.
 virtual std::optional<Snapshot> SnapshotOriginalOffer(Client&,NPC&,const Lease&,ZoneDatabase&)=0;
 // Inside writer intent transaction. Exact immutable replay match or refuse.
 // The planner is recomputed from locked state, original offers and bound
 // parameters; intent and physical full-return plan commit before any debit.
 virtual bool PrepareIntent(ZoneDatabase&,const Intent&)=0;
 // Inside SAME strict transaction as physical inventory/currency/return-state
 // persistence. CAS carry/revision + native absolute activity count + action0→1.
 // No COMMIT/ROLLBACK here; the writer owns the exclusive transaction outcome.
 virtual bool CommitDebitAndProgress(ZoneDatabase&,const Intent&)=0;
 // Only definite physical commit AND native ownership settlement can issue this
 // proof. Spells reads action1/current occurrence, projects durable target count,
 // saves/readbacks native transition then CASes1→2. Action2 is idempotent.
 virtual Ack AcknowledgeProgress(Client&,NPC&,const CommittedAndSettled&)=0;
 // Only confirmed physical full return issues this proof. Action0→3, no credit.
 // Unknown commit never invokes it; retained lease recovery owns the decision.
 virtual bool MarkReturned(ZoneDatabase&,const ConfirmedFullReturn&)=0;
};
}
