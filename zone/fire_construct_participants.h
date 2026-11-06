#pragma once
#include "fire_personal_item_use.h"
#include <memory>
class Client;
class NPC;
class TaskManager;
namespace FirePersonalOccurrenceSource {
class OriginalGotonurSayLifetime;
class PreparedConstructPlan;
class NativeConstructParticipants;
class OriginalConstructDeathQueue;
class ConstructDeathReceiptStore;
inline bool UsesBoundedNativeGroupCredit(uint32_t task_id) noexcept {return task_id==920168 || task_id==920169;}
struct NativeConstructParticipantVisit {
 friend class NativeConstructParticipants;
 friend class OriginalConstructDeathQueue;
 friend class ConstructDeathReceiptStore;
private:
 Binding recipient_{};
 Client* original_client_=nullptr; // compare only with freshly resolved live owner
 std::shared_ptr<OriginalGotonurSayLifetime> client_lifetime_;
 uint32_t client_entity_=0,group_id_=0;
 bool observed_=false,submitted_=false,saved_=false,persisted_=false;
public:
 NativeConstructParticipantVisit()=default; // no authority until actual native visit
};
class NativeConstructParticipants final {
 friend class ::TaskManager;
 friend class PreparedConstructPlan;
 static std::optional<Binding> OwnEligibleOccurrence(Client&,uint32_t);
 // Actual existing task-kill candidate loop only, before its mutable task call.
 // An unsealed call cannot itself deliver progress or create a persisted receipt.
 static void ObserveActualTaskKillVisit(Client& candidate,Client& actual_exp_client,NPC& actual_actor);
};
} // namespace FirePersonalOccurrenceSource
