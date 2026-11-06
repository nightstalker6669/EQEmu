#pragma once
#include "fire_original_construct_death.h"
#include <memory>
#include <string>
namespace FireRunActionSource {class RunUseWitness;class NativeRunDoorSource;class PublishedRunActionSink;class NativeRunHelperSource;class RunDoorLifetime;class RunCorpseLifetime;class MissionMaterialCorpseOrigin;class OriginalMissionMaterialDeath;class NativeRunLootSource;}
namespace FirePersonalOccurrenceSource {
inline constexpr uint32_t OriginalGotonurStaticSpawn=149749; // qualified static receipt, not live identity
class OriginalGotonurSayQueue;
class OriginalGotonurSayLifetime final {
 friend class OriginalGotonurSayQueue;
 friend class FireRunActionSource::RunUseWitness;
 friend class NativeConstructParticipants;
 friend class OriginalConstructDeathQueue;
 friend class ::Client;
 friend class ::NPC;
 bool active_=true;
 OriginalGotonurSayLifetime()=default;
 OriginalGotonurSayLifetime(const OriginalGotonurSayLifetime&)=delete;
 OriginalGotonurSayLifetime& operator=(const OriginalGotonurSayLifetime&)=delete;
public:
 ~OriginalGotonurSayLifetime()=default;
};
class OriginalGotonurSayQueue final {
 friend class ::EntityList;
 friend class ::Zone;
 struct Pending {
  std::shared_ptr<OriginalGotonurSayLifetime> client_lifetime,npc_lifetime;
  std::shared_ptr<OriginalConstructDeathQueue::Lifetime> zone_lifetime;
  Client* original_client;
  NPC* original_npc;
  uint32_t character_id,client_entity,npc_entity;
  std::string message;
  Pending* next=nullptr;
 };
 static Pending* head_;
 static Pending* tail_;
 static bool MatchesActualContext(Client&,NPC&);
 static void Drain();
 static void Discard() noexcept;
public:
 // Capture before ordinary mutable say callbacks. This queues only the genuine
 // say intent; native admission/death-receipt checks remain at actual dispatch.
 static void CaptureActual(Client&,const char* actual_message);
};
} // namespace FirePersonalOccurrenceSource
