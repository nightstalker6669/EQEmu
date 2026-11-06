// SOURCE ONLY: feature-private companion to corrected acceptance patch b71dc874.
// Not included, built, installed or invoked. No callback registration/public API.
#pragma once
#include "fire_personal_successor_dispatch.h"
#include "fire_shared_successor_native.h"
namespace FirePersonalOccurrenceSource {
enum class OriginalFinishResult { Hold, SubmittedWithoutSuccessorReceipt, SuccessorAcceptedBeforeCallbacks };
class OriginalFinalHailAttempt {
 Binding original_;
 bool attempted_=false;
 friend OriginalFinishResult SubmitActualOriginalFinalHailOnce(Database&,OriginalFinalHailAttempt&,
  Client&,TaskType,const ClientTaskInformation&,NPC&,uint32_t,uint32_t,uint32_t);
public:
 explicit OriginalFinalHailAttempt(const Binding& original):original_(original) {}
 OriginalFinalHailAttempt(const OriginalFinalHailAttempt&)=delete;
 OriginalFinalHailAttempt& operator=(const OriginalFinalHailAttempt&)=delete;
};
struct OriginalFinishFrame {
 Binding original;
 Client* client;
 NPC* original_actor; // comparison only after mutable callbacks; never dereference the retained pointer.
 uint32_t actor_entity,original_spawn;
 bool entered=false,removal_entered=false,save_seen=false,cleanup_ready=false,dispatch_attempted=false;
 ActualOriginalCleanup cleanup;
 std::optional<Handoff> handoff;
 OriginalFinishResult result=OriginalFinishResult::SubmittedWithoutSuccessorReceipt;
 OriginalFinishFrame(const Binding& b,Client& c,NPC& n,uint32_t spawn)
  :original(b),client(&c),original_actor(&n),actor_entity(n.GetID()),original_spawn(spawn) {}
};
inline thread_local OriginalFinishFrame* original_finish_frame=nullptr;
class OriginalFinishScope {
 OriginalFinishFrame* previous_;
public:
 explicit OriginalFinishScope(OriginalFinishFrame& f):previous_(original_finish_frame) { original_finish_frame=&f; }
 ~OriginalFinishScope() { original_finish_frame=previous_; } // no native object access
 OriginalFinishScope(const OriginalFinishScope&)=delete;
 OriginalFinishScope& operator=(const OriginalFinishScope&)=delete;
};
inline OriginalFinishFrame* ClaimOriginalFinalUpdate(Client* client,TaskType type,
 const ClientTaskInformation& current,int goal,int count) {
 auto* f=original_finish_frame;
 if(!f || f->entered || f->client!=client || type!=TaskType::Quest || goal!=3 || count!=1 ||
  !MatchesLocal(f->original,client->CharacterID(),type,current) ||
  current.activity[3].activity_state!=ActivityActive ||
  current.activity[2].activity_state!=ActivityCompleted) return nullptr;
 f->entered=true;return f;
}
inline void ObserveOriginalRewardCall(OriginalFinishFrame* f,Database& db,Client* client,
 TaskType type,const ClientTaskInformation& current,const ActualOriginalRewardCall& call) {
 if(!f || f->client!=client || f->handoff) return;
 auto intent=AfterOriginalRewardTask(db,f->original,client->CharacterID(),type,current,call);
 if(intent) f->handoff.emplace(*intent); // before native mapping removal, no replacement on retry
}
inline OriginalFinishFrame* ClaimOriginalRemoval(Client* client,int task,int slot,TaskType type) {
 auto* f=original_finish_frame;
 if(!f || !f->entered || !f->handoff || f->removal_entered || f->client!=client ||
  type!=TaskType::Quest || task!=static_cast<int>(f->original.task_id) || slot!=f->original.slot)
  return nullptr;
 f->removal_entered=true;
 f->cleanup.character=f->original.character_id;
 f->cleanup.finished_task=f->original.task_id;
 f->cleanup.original_occurrence=f->original.occurrence_id;
 f->cleanup.original_slot=f->original.slot;
 f->cleanup.original_removal_executed=true;
 return f;
}
inline void ObserveOriginalRemoval(OriginalFinishFrame* f,int activities_removed,int tasks_removed,
 bool pointer_removed,bool actual_slot_free) {
 if(!f) return;
 // Existing repository DeleteWhere returns zero on failure; only exact positive counts qualify.
 f->cleanup.task_and_activity_removal_known=activities_removed==4 && tasks_removed==1;
 f->cleanup.original_pointer_removal_known=pointer_removed;
 f->cleanup.original_slot_now_free=actual_slot_free;
}
inline void ObserveOriginalCompletionSave(OriginalFinishFrame* f,bool actual_existing_save_result) {
 if(!f || !f->handoff || !f->removal_entered || f->save_seen) return;
 f->save_seen=true;
 f->cleanup.original_completion_save_executed=true;
 f->cleanup.original_completion_save_succeeded=actual_existing_save_result;
 f->cleanup_ready=ConfirmOriginalCleanup(*f->handoff,f->cleanup);
}
inline void DispatchAfterOriginalCompletion(OriginalFinishFrame* f,Client* actual_client,
 NPC* actual_actor_now,uint32_t actual_zone,uint32_t actual_instance) {
 if(!f || !f->cleanup_ready || !f->handoff || f->dispatch_attempted ||
  f->client!=actual_client || !actual_actor_now || actual_actor_now!=f->original_actor ||
  actual_actor_now->GetID()!=f->actor_entity || actual_actor_now->GetNPCTypeID()!=490061 ||
  actual_actor_now->GetSpawnPointID()!=f->original_spawn || actual_zone!=490 || actual_instance!=0)
  return;
 f->dispatch_attempted=true;
 if(f->original.task_id==920169) {
  SharedSuccessorNative::Dispatch(*f->handoff,f->original,*actual_client,*actual_actor_now,
   actual_zone,actual_instance,f->original_spawn);
  return; // native World admission, never one DZ/create per recipient
 }
 const auto result=DispatchActualOriginalPersonalSuccessorOnce(*f->handoff,f->original,
  *actual_client,*actual_actor_now,actual_zone,actual_instance,f->original_spawn);
 // Read/write stack state only after mutable successor callbacks.
 if(result==PersonalDispatchResult::AcceptedBeforeCallbacks)
  f->result=OriginalFinishResult::SuccessorAcceptedBeforeCallbacks;
}
inline OriginalFinishResult SubmitActualOriginalFinalHailOnce(Database& db,OriginalFinalHailAttempt& attempt,
 Client& actual_client,TaskType actual_type,const ClientTaskInformation& current,NPC& actual_actor,
 uint32_t actual_zone,uint32_t actual_instance,uint32_t selected_original_spawn) {
 if(original_finish_frame || attempt.attempted_ ||
  (attempt.original_.task_id!=920167 && attempt.original_.task_id!=920168 && attempt.original_.task_id!=920169) ||
  actual_zone!=490 || actual_instance!=0 || actual_actor.GetNPCTypeID()!=490061 ||
  !actual_actor.GetID() || !selected_original_spawn ||
  actual_actor.GetSpawnPointID()!=selected_original_spawn ||
  current.activity[2].activity_state!=ActivityCompleted ||
  !ForConsumer(db,attempt.original_,actual_client.CharacterID(),actual_type,current,Consumer::FinalHail))
  return OriginalFinishResult::Hold;
 // Only the actual original hail owner may call, after its authentic construct/death consumer.
 // Current activity counters do NOT manufacture physical encounter ownership/commit evidence.
 attempt.attempted_=true;
 OriginalFinishFrame frame(attempt.original_,actual_client,actual_actor,selected_original_spawn);
 OriginalFinishScope scope(frame);
 actual_client.UpdateTaskActivity(attempt.original_.task_id,3,1);
 return frame.result; // no client/NPC/current/handoff dereference after callbacks
}
} // namespace FirePersonalOccurrenceSource
