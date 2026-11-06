#pragma once
#include "fire_original_construct_producer.h"
#include <memory>
class Zone;
namespace FireRunActionSource {class RunUseWitness;class NativeRunDoorSource;class PublishedRunActionSink;class NativeRunHelperSource;class RunDoorLifetime;class RunCorpseLifetime;class MissionMaterialCorpseOrigin;class OriginalMissionMaterialDeath;class NativeRunLootSource;}
namespace FirePersonalOccurrenceSource {
class OriginalConstructNativeDeath;
// Finite native-event queue. No timer, retry, history scan or public script API.
class OriginalConstructDeathQueue final {
 friend class OriginalConstructNativeDeath;
 friend class OriginalGotonurSayQueue;
 friend class FireRunActionSource::RunUseWitness;
 friend class ::EntityList;
 friend class ::Zone;
 struct Lifetime {const Zone* owner;bool active=true;};
 static std::shared_ptr<Lifetime> lifetime_;
 static std::shared_ptr<Lifetime> ForActualZone(const Zone*);
 struct Pending {
  std::shared_ptr<OriginalConstructAttempt> attempt;
  std::shared_ptr<Lifetime> lifetime;
  const Zone* zone_owner;
  uint32_t zone_id,instance_id;
  Pending* next=nullptr;
 };
 static Pending* head_;
 static Pending* tail_;
 static void Push(Pending*) noexcept;
 static void Drain(); // at end of normal EntityList::MobProcess, outside actor stacks
 static void Discard() noexcept; // Zone destruction: no late event recreation
};
class OriginalConstructNativeDeath final {
 friend class ::NPC;
 std::shared_ptr<OriginalConstructAttempt> attempt_;
 std::unique_ptr<OriginalConstructDeathQueue::Pending> pending_;
 NPC* actor_=nullptr; // used only during the genuine owning NPC::Death call
 uint32_t killed_entity_=0,zone_=0,instance_=0;
 bool entered_=false,completed_=false;
 explicit OriginalConstructNativeDeath(NPC&);
 void SealSuccess(NPC&,uint32_t actual_native_killed_entity);
public:
 ~OriginalConstructNativeDeath();
 OriginalConstructNativeDeath(const OriginalConstructNativeDeath&)=delete;
 OriginalConstructNativeDeath& operator=(const OriginalConstructNativeDeath&)=delete;
 OriginalConstructNativeDeath(OriginalConstructNativeDeath&&)=delete;
 OriginalConstructNativeDeath& operator=(OriginalConstructNativeDeath&&)=delete;
};
} // namespace FirePersonalOccurrenceSource
