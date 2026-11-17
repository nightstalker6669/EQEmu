#include "client.h"
#include "npc.h"
#include "zone.h"
#include "zonedb.h"
#include "task_client_state.h"
#include "task_manager.h"
#include "original_trade_offer.h"
#include "original_trade_transform_hooks.h"
#include "common/sod_original_transform_native.h"
#include "common/sod_task_activation.h"

namespace OriginalTradeTransforms {
struct NativeTaskAccess {
static ClientTaskInformation* Find(Client& c,uint32_t task) {
 auto* state=c.GetTaskState();if(!state)return nullptr;
 if(task==920197){auto* a=state->GetClientTaskInfo(TaskType::Shared,TASKSLOTSHAREDTASK);return a&&a->task_id==task?a:nullptr;}
 ClientTaskInformation* found=nullptr;
 for(int i=0;i<MAXACTIVEQUESTS;++i){auto* a=state->GetClientTaskInfo(TaskType::Quest,i);
  if(a&&a->task_id==task){if(found)return nullptr;found=a;}}
 return found;
}
};
namespace S=SodOriginalTransform;
namespace {
struct Route {uint32_t task,activity,npc,spawn,input,output;X::Output operation;};
constexpr Route FirstSkull{920197,1,452108,129109,88326,88326,X::Output::ReturnOriginal};
constexpr Route Tails{920197,0,452108,129109,88325,0,X::Output::ConsumeOnly};
constexpr Route Ribcage{920207,1,452150,129220,17381,17381,X::Output::ReturnOriginal};
constexpr Route Tar{920207,7,452133,129172,50004,37763,X::Output::CreateBound};
constexpr Route Calethos{920207,9,452918,0,37763,17388,X::Output::CreateBound};
ClientTaskInformation* Active(Client& c,uint32_t task) { return NativeTaskAccess::Find(c,task); }
bool Base() {return zone&&zone->GetZoneID()==452&&!zone->GetInstanceID()&&!zone->GetInstanceVersion();}
bool HasInput(const X::T::Offers& offers,uint32_t id) {
 return std::any_of(offers.begin(),offers.end(),[&](const auto& o){return o.present&&o.item_id==id;});
}
const Route* Candidate(Client& c,NPC& npc,const X::T::Offers& offers) {
 // TYPE candidates are intercepted even at a wrong SPID. Only Snapshot may
 // authorize the exact original actor; fall-through must not consume inputs.
 if(npc.GetNPCTypeID()==FirstSkull.npc&&Active(c,920197)){
  if(c.IsTaskActivityActive(920197,1)&&HasInput(offers,88326))return &FirstSkull;
  if(c.IsTaskActivityActive(920197,0)&&HasInput(offers,88325))return &Tails;
 }
 if(npc.GetNPCTypeID()==Ribcage.npc&&c.IsTaskActivityActive(920207,1)&&HasInput(offers,17381))return &Ribcage;
 if(npc.GetNPCTypeID()==Tar.npc&&c.IsTaskActivityActive(920207,7)&&HasInput(offers,50004))return &Tar;
 if(npc.GetNPCTypeID()==Calethos.npc&&c.IsTaskActivityActive(920207,9)&&HasInput(offers,37763))return &Calethos;
 return nullptr; // final skull197/3 stays ordinary final native delivery
}
struct Witness {Client* actor=nullptr;ClientTaskInformation* info=nullptr;uint64_t occurrence=0;uint32_t accepted=0,activity=0;};
thread_local Witness personal_witness;
}

bool ClaimsOffer(Client& c,NPC& npc) {
 // Do not condition interception on zone/version/instance, feature flag or
 // SPID. A wrong-location TYPE/item/stage candidate is still protected before
 // generic consumption; SnapshotOriginal alone authorizes the exact provider.
 // A disabled/unverified candidate is returned by the original writer, never consumed by
 // an optimistic generic delivery before World rejects the protected route.
 const auto offers=OriginalTradeOffer::Capture(c);
 const auto type=npc.GetNPCTypeID();
 if(type==452079||type==452108||type==452121||type==452133||type==452147||type==452150)
  for(const auto& root:offers)if(root.present&&X::MessageTask(root.item_id))return true;
 return Candidate(c,npc,offers)!=nullptr;
}

std::optional<X::Context> SnapshotOriginal(Client& c,NPC& npc,const OriginalTransformNative::Lease& lease,ZoneDatabase& db) {
 try{
  S::Strict(db);
  if(!Base()||!RuleB(SodProgression,EnableFieldOfScaleSlice)||!lease.active||lease.actor.character_id!=c.CharacterID()||
   lease.actor.account_id!=c.AccountID()||lease.owner.zone_id!=452||lease.owner.instance_id||
   !c.HasInventorySessionReservation()||!c.trade||c.trade->With()!=&npc||!npc.GetID()||npc.GetDepop()||npc.GetHP()<=0)return std::nullopt;
  const auto offers=OriginalTradeOffer::Capture(c);
  unsigned notes=0,message_primary=0;
  for(unsigned i=0;i<offers.size();++i)if(offers[i].present&&X::MessageTask(offers[i].item_id)){++notes;message_primary=i;}
  if(notes) {
   if(notes!=1||!X::MessageGiver(npc.GetNPCTypeID(),npc.GetSpawnPointID())||X::T::Available(offers[message_primary])!=1)return std::nullopt;
   const auto task=X::MessageTask(offers[message_primary].item_id);
   if(!SodProgression::MayStartOrJoinTask(content_db,task,true)||!TaskManager::Instance()->CanOfferOriginalMessage(&c,&npc,task))return std::nullopt;
   X::Context out;out.character=c.CharacterID();out.account=c.AccountID();out.task=task;out.model=S::Model;out.shared=true;
   // No active occurrence/activity exists before original note admission.
   out.npc_type=npc.GetNPCTypeID();out.npc_entity=npc.GetID();out.npc_spawn=npc.GetSpawnPointID();out.zone=452;
   out.input_item=offers[message_primary].item_id;out.primary=message_primary;out.output=X::Output::ConsumeOnAdmission;out.offers=offers;out.requested[message_primary]=1;
   return X::Valid(out)?std::optional<X::Context>(out):std::nullopt;
  }
  const auto* route=Candidate(c,npc,offers);
  if(!route||npc.GetSpawnPointID()!=route->spawn)return std::nullopt;
  if(route==&Calethos) {
   TimeOfDay_Struct clock{};zone->zone_time.GetCurrentEQTimeOfDay(time(nullptr),&clock);
   const auto hour=clock.hour-1;
   if((hour<22&&hour>=2)||npc.GetEntityVariable("sod_fos_calethos")!="original-clock"||
    entity_list.GetNPCByNPCTypeID(Calethos.npc)!=&npc)return std::nullopt;
  }
  auto* info=Active(c,route->task);
  if(!info||info->accepted_time<=0||uint64_t(info->accepted_time)>UINT32_MAX)return std::nullopt;
  const bool shared=route->task==920197;
  if(!SodProgression::MayStartOrJoinTask(content_db,route->task,shared))return std::nullopt;
  const auto* task=TaskManager::Instance()->GetTaskData(route->task);
  if(!task||task->type!=(shared?TaskType::Shared:TaskType::Quest)||route->activity>=uint32_t(task->activity_count)||
   task->activity_information[route->activity].activity_type!=TaskActivityType::Deliver||
   task->activity_information[route->activity].goal_count!=(route->activity==0?4:1))return std::nullopt;
  X::Context out;out.character=c.CharacterID();out.account=c.AccountID();out.task=route->task;out.activity=route->activity;
  out.accepted_time=uint32_t(info->accepted_time);out.model=S::Model;out.shared=shared;
  out.npc_type=npc.GetNPCTypeID();out.npc_entity=npc.GetID();out.npc_spawn=npc.GetSpawnPointID();out.zone=452;
  out.input_item=route->input;out.output_item=route->output;out.output=route->operation;out.offers=offers;
  if(shared){
   if(c.GetSharedTaskId()<=0||info->activity[0].done_count<0||info->activity[0].done_count>4)return std::nullopt;
   out.occurrence=uint64_t(c.GetSharedTaskId());out.before_tail=uint32_t(info->activity[0].done_count);
   if(!S::ActiveShared(db,out,false))return std::nullopt; // World repeats authoritatively under native locks.
  }else{
   auto q=S::Q(db,fmt::format("SELECT occurrence_id,accepted_time FROM character_task_reward_instances "
    "WHERE character_id={} AND task_id=920207 FOR UPDATE",out.character));
   if(q.RowCount()!=1||S::N(q.begin()[1])!=out.accepted_time)return std::nullopt;
   out.occurrence=S::N(q.begin()[0]);
   S::Personal(db,out,0,true); // includes prior ribcage/fire custody; never debit them again at Tar.
  }
  bool primary=false;
  for(unsigned i=0;i<4;++i)if(offers[i].present&&offers[i].item_id==route->input&&X::T::Available(offers[i])){
   out.primary=i;out.activities[i]=route->activity;
   out.requested[i]=route->activity==0?std::min(4-out.before_tail,X::T::Available(offers[i])):1;
   primary=out.requested[i]!=0;if(primary)break;
  }
  if(!primary)return std::nullopt;
  if(shared&&c.IsTaskActivityActive(920197,0)){
   unsigned remaining=4-out.before_tail;
   if(route->activity==0)remaining-=out.requested[out.primary];
   for(unsigned i=0;i<4&&remaining;++i)if(i!=out.primary&&offers[i].present&&offers[i].item_id==88325){
    out.activities[i]=0;out.requested[i]=std::min(remaining,X::T::Available(offers[i]));remaining-=out.requested[i];
   }
  }
  if(!X::Valid(out))return std::nullopt;
  if(!shared)personal_witness={&c,info,out.occurrence,out.accepted_time,out.activity};
  return out;
 }catch(...){return std::nullopt;}
}

void SubmitSharedSameRequest(const std::string&,const X::Context&) {
 // Optional notification only. World's durable outbox worker is authoritative;
 // no opcode, transient message or client-provided key is needed for progress.
}

bool Acknowledge(Client& c,const Committed& proof) {
 const auto& context=proof.Context();
 if(c.CharacterID()!=context.character||c.AccountID()!=context.account)return false;
 // Personal proofs here originate synchronously from the same original accept
 // after SnapshotOriginal, with no quest callback between snapshot and proof.
 // Shared continuations use a separate path; an old recovered personal receipt
 // must not acquire this consumed one-use runtime witness.
 const auto witness=personal_witness;personal_witness={};
 auto* info=Active(c,context.task);
 if(!context.shared&&witness.actor==&c&&witness.info==info&&witness.occurrence==context.occurrence&&
  witness.accepted==context.accepted_time&&witness.activity==context.activity&&info&&
  uint32_t(info->accepted_time)==context.accepted_time&&info->activity[context.activity].done_count>=0&&info->activity[context.activity].done_count<=1){
  // Allocation-free scalar publication precedes encoding, SQL and packet/save
  // work. The exact task-save guard protects all other uncertain cases.
  info->activity[context.activity].done_count=1;info->activity[context.activity].activity_state=ActivityCompleted;
  info->activity[context.activity].updated=true;
 }
 try{
  const auto physical=S::ReadPhysical(database,proof.Key(),context,false);
  const auto decision=S::ReadFinal(database,proof.Key(),context,false);
  if(physical.outcome!=1||physical.settled!=1||decision.state!=X::DecisionState::Known)return false;
  if(context.shared){
   // The immutable decision settles THIS request's roots. It is not a current
   // count snapshot: World publishes latest canonical same-run counts only
   // after physical settlement. Never regress a newer member contribution.
   return true;
  }
  info=Active(c,920207);
  if(!info||uint32_t(info->accepted_time)!=context.accepted_time)return true; // old property owed, no new task projection
  auto pointer=S::Q(database,fmt::format("SELECT occurrence_id,accepted_time FROM character_task_reward_instances "
   "WHERE character_id={} AND task_id=920207",context.character));
  if(pointer.RowCount()!=1||S::N(pointer.begin()[0])!=context.occurrence||S::N(pointer.begin()[1])!=context.accepted_time)return true;
  S::Personal(database,context,1,false);
  if(info->activity[context.activity].done_count<0||info->activity[context.activity].done_count>1)return false;
  info->activity[context.activity].done_count=1;info->activity[context.activity].activity_state=ActivityCompleted;
  info->activity[context.activity].updated=true;
  c.UpdateTaskActivity(920207,context.activity,0,true); // native transition only, no repeated increment/output
  return true;
 }catch(...){return false;}
}
}
