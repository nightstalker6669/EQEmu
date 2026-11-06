#pragma once
#include "fire_original_construct_producer.h"
#include <memory>
namespace FirePersonalOccurrenceSource {
class ConstructDeathReceiptStore;
class PersistedConstructFinish final {
 friend class ConstructDeathReceiptStore;
 friend class OriginalConstructNativeHail;
 Binding original_;
 OriginalFinalHailAttempt hail_;
 Client* owner_;
 uint32_t giver_entity_,giver_spawn_;
 bool used_=false;
 PersistedConstructFinish(const Binding& b,Client& c,NPC& n)
  :original_(b),hail_(b),owner_(&c),giver_entity_(n.GetID()),giver_spawn_(n.GetSpawnPointID()) {}
 void Submit(Client&,NPC&,const ClientTaskInformation&);
public:
 PersistedConstructFinish(const PersistedConstructFinish&)=delete;
 PersistedConstructFinish& operator=(const PersistedConstructFinish&)=delete;
};
class ConstructDeathReceiptStore final {
 friend class OriginalConstructNativeDeath;
 friend class OriginalConstructDeathQueue;
 friend class OriginalConstructNativeHail;
 // No public write/factory/context-only proof. Only actual native death scope
 // can persist, after its SAME original goal2 save was positively observed.
 static bool PersistActualDeath(Database&,const OriginalConstructAttempt&);
 static bool PersistActualParticipant(Database&,const OriginalConstructAttempt&,const NativeConstructParticipantVisit&);
 static bool PersistSavedRecipient(Database&,const OriginalConstructAttempt&,const Binding&);
 static std::shared_ptr<PersistedConstructFinish> ClaimForActualHail(
  Database&,Client&,NPC&,const ClientTaskInformation&);
};
} // namespace FirePersonalOccurrenceSource
