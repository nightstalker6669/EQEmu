// OFFLINE SOURCE PROPOSAL ONLY. No NPC entry is registered by this header.
#pragma once
#include "common/fire_shared_correlation.h"
#include "fire_personal_after_reward_r2.h"
#include <memory>
class Client;
class NPC;
namespace FirePersonalOccurrenceSource {
struct SharedSuccessorPending {
 Binding source;
 uint64_t reward=0,local_occurrence=0;
 uint32_t giver=0,spawn=0;
 const NPC* original_actor=nullptr; // compare only, never dereference retained pointer
 bool stamped=false,accept_claimed=false,save_seen=false,local_saved=false,reloaded=false;
 bool ready_received=false,ready=false,held=false;
 ServerSharedTaskRequest_Struct accepted{};
 FireSharedCorrelation::Trailer world{};
};
enum class CorrelatedSharedResult { Hold,NativeInvocationUnconfirmed };
class SharedSuccessorNative {
 static bool Match(const SharedSuccessorPending&,const ServerSharedTaskRequest_Struct&,
  const FireSharedCorrelation::Trailer&);
 static void Finish(Client&);
public:
 static CorrelatedSharedResult Dispatch(Handoff&,const Binding&,Client&,NPC&,uint32_t,uint32_t,uint32_t);
 static bool Stamp(Client&,ServerSharedTaskRequest_Struct&,FireSharedCorrelation::Trailer&);
 static bool BeginAccept(Client&,const ServerSharedTaskRequest_Struct&,const FireSharedCorrelation::Trailer&);
 static void ObserveReservation(Client&,int,uint32_t,uint64_t);
 static void ObserveSave(Client&,TaskType,const ClientTaskInformation&,bool,int);
 static void AfterReload(Client&);
 static void ReadyOrHeld(Client&,const ServerSharedTaskRequest_Struct&,const FireSharedCorrelation::Trailer&);
};
} // namespace FirePersonalOccurrenceSource
