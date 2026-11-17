#include "client.h"
#include "npc.h"
#include "zone.h"
#include "task_client_state.h"
#include "sod_food_delivery.h"
#include "original_trade_offer.h"
#include "common/sod_food_binding.h"
#include "common/sod_weighted_delivery_context.h"
#include "common/sod_weighted_delivery_recovery.h"
#include "common/rulesys.h"
#include <ctime>
#include <stdexcept>

extern Zone* zone;
namespace SodFoodDelivery {
// Narrow access preserves absolute publication into the existing native slot.
struct NativeTaskAccess {
static ClientTaskInformation* Find(Client& c) {
 auto* state=c.GetTaskState();if(!state)return nullptr;
 ClientTaskInformation* found=nullptr;
 for(int index=0;index<MAXACTIVEQUESTS;++index) {
  auto* info=state->GetClientTaskInfo(TaskType::Quest,index);
  if(info && info->task_id==920198){if(found)return nullptr;found=info;}
 }
 return found;
}
};
namespace {
namespace W=EQEmu::SharedCore::WeightedPersonalDelivery;
namespace R=SodWeightedDeliveryRecovery;
using namespace OriginalTradeWriter;
constexpr uint32_t Task=920198,Activity=1;
auto Query(Database& db,const std::string& sql) {
 auto r=db.QueryDatabase(sql);if(!r.Success())throw std::runtime_error("Food semantic persistence unavailable");return r;
}
uint64_t Number(const char* value) {
 const auto n=SharedTaskAdmission::Number(value);
 if(!n)throw std::runtime_error("Food semantic numeric evidence differs");return *n;
}
void Strict(Database& db) {
 auto r=Query(db,"SELECT @@in_transaction");
 if(r.RowCount()!=1 || Number(r.begin()[0])!=1 || db.TransactionStrictFailed())
  throw std::runtime_error("Food semantic hook requires the writer strict transaction");
}
ClientTaskInformation* Active(Client& c) { return NativeTaskAccess::Find(c); }
// The pointer is allocated by native acceptance. NEVER allocate/REPLACE here.
W::Occurrence Native(Database& db,uint32_t who,uint32_t accepted,uint32_t progress,bool lock) {
 auto r=Query(db,fmt::format(
  "SELECT r.occurrence_id,r.accepted_time,t.acceptedtime,t.type,t.was_rewarded,a.donecount,a.completed "
  "FROM character_task_reward_instances r JOIN character_tasks t ON t.charid=r.character_id AND t.taskid=r.task_id "
  "JOIN character_activities a ON a.charid=t.charid AND a.taskid=t.taskid AND a.activityid=1 "
  "WHERE r.character_id={} AND r.task_id=920198{}",who,lock?" FOR UPDATE":""));
 if(r.RowCount()!=1)throw std::runtime_error("Food active personal occurrence unavailable");
 auto row=r.begin();W::Occurrence out;
 out.character_id=who;out.occurrence_id=Number(row[0]);out.accepted_time=uint32_t(Number(row[1]));
 out.task_id=Task;out.activity_id=Activity;out.progress=uint32_t(Number(row[5]));
 if(!out.occurrence_id || out.accepted_time!=accepted || Number(row[2])!=accepted ||
  Number(row[3])!=2 || Number(row[4]) || out.progress!=progress || progress>10 ||
  Number(row[6])!=(progress==10?1u:0u))throw std::runtime_error("Food personal occurrence/activity changed");
 return out;
}
void Carry(Database& db,W::Occurrence& o,bool may_be_absent) {
 auto r=Query(db,fmt::format(
  "SELECT recipe_model,revision,carry_pounds,carry_expires_at FROM sod_weighted_delivery_state "
  "WHERE character_id={} AND task_id=920198 AND occurrence_id={} AND activity_id=1 FOR UPDATE",
  o.character_id,o.occurrence_id));
 if(r.RowCount()==0 && may_be_absent && o.progress==0)return;
 if(r.RowCount()!=1)throw std::runtime_error("Food carry state unavailable");
 auto row=r.begin();if(Number(row[0])!=SodFoodBinding::Model)throw std::runtime_error("Food carry model differs");
 o.revision=Number(row[1]);const auto carry=Number(row[2]),expiry=Number(row[3]);
 if(carry>=10 || expiry>UINT32_MAX || (!carry&&expiry) || (carry&&!expiry) || o.revision==UINT64_MAX)
  throw std::runtime_error("Food carry snapshot invalid");
 o.carry_pounds=uint32_t(carry);o.carry_expires_at=expiry;
}
bool Same(const W::Occurrence& a,const W::Occurrence& b) {
 return a.character_id==b.character_id && a.occurrence_id==b.occurrence_id &&
  a.accepted_time==b.accepted_time && a.task_id==b.task_id && a.activity_id==b.activity_id &&
  a.progress==b.progress && a.carry_pounds==b.carry_pounds &&
  a.carry_expires_at==b.carry_expires_at && a.revision==b.revision;
}
std::string Context(const Intent& i) {return SodWeightedDeliveryContext::Encode(i.snapshot);}
void Preimage(ZoneDatabase& db,const Intent& i) {
 const auto& before=i.snapshot.occurrence;
 auto current=Native(db,before.character_id,before.accepted_time,before.progress,true);
 Carry(db,current,false);if(!Same(current,before))throw std::runtime_error("Food immutable preimage changed");
}
class Recipe final:public SemanticRecipe {
public:
 std::optional<Snapshot> SnapshotOriginalOffer(Client& c,NPC& npc,const Lease& lease,ZoneDatabase& db) override {
  try {
   Strict(db);
   if(!SodFoodBinding::Enabled || !ClaimsOffer(c,npc) || !lease.active ||
    lease.actor.character_id!=c.CharacterID() || lease.owner.zone_id!=452 || lease.owner.instance_id ||
    !c.HasInventorySessionReservation() || !c.trade || c.trade->With()!=&npc ||
    !npc.GetID() || npc.GetDepop() || npc.GetHP()<=0 ||
    !c.IsTaskActivityActive(Task,Activity))return std::nullopt;
   auto* info=Active(c);if(!info || info->accepted_time<=0 || info->activity[Activity].done_count<0 ||
    info->activity[Activity].done_count>=10)return std::nullopt;
   Snapshot out;
   out.occurrence=Native(db,c.CharacterID(),uint32_t(info->accepted_time),uint32_t(info->activity[Activity].done_count),true);
   Carry(db,out.occurrence,true);
   auto prior=Query(db,fmt::format("SELECT request_key FROM sod_weighted_delivery_actions WHERE character_id={} AND task_id=920198 AND occurrence_id={} AND activity_id=1 AND status=1 LIMIT 1 FOR UPDATE",c.CharacterID(),out.occurrence.occurrence_id));
   if(prior.RowCount()!=0)return std::nullopt; // Core reconciles known settled prior acknowledgement BEFORE entering a new writer transaction.
   out.parameters=SodFoodBinding::Parameters();out.offers=OriginalTradeOffer::Capture(c);
   out.npc={npc.GetNPCTypeID(),npc.GetID(),npc.GetSpawnPointID(),452,0,0};
   out.recipe_model=SodFoodBinding::Model;out.observed_at=uint64_t(std::time(nullptr));
   if(W::Prepare(out.parameters,out.occurrence,out.offers,out.observed_at,out.plan)!=W::Status::Ready ||
    !SodWeightedDeliveryContext::Valid(out))return std::nullopt;
   return out;
  }catch(...){return std::nullopt;}
 }
 bool PrepareIntent(ZoneDatabase& db,const Intent& i) override {
  try {
   Strict(db);const auto bytes=Context(i);const auto& b=i.snapshot.occurrence;
   if(!R::Valid(SodWeightedDeliveryContext::Identity(i.snapshot),i.request_key,bytes))return false;
   // Absence is valid only for a fresh zero-progress occurrence. This insert
   // and semantic intent commit in the SAME transaction as physical intent.
   auto current=Native(db,b.character_id,b.accepted_time,b.progress,true);Carry(db,current,true);
   if(!Same(current,b))return false;
   Query(db,fmt::format("INSERT IGNORE INTO sod_weighted_delivery_state "
    "(character_id,task_id,occurrence_id,activity_id,recipe_model,revision,carry_pounds,carry_expires_at) "
    "VALUES ({},920198,{},1,{},0,0,0)",b.character_id,b.occurrence_id,i.snapshot.recipe_model));
   Preimage(db,i);
   const auto prior=R::Inspect(db,SodWeightedDeliveryContext::Identity(i.snapshot),i.request_key,bytes);
   if(prior==R::State::Prepared)return true;
   if(prior!=R::State::Missing)return false;
   auto r=Query(db,fmt::format("INSERT INTO sod_weighted_delivery_actions "
    "(request_key,character_id,task_id,occurrence_id,activity_id,accepted_time,recipe_model,context_blob,status,updated_at) "
    "VALUES ('{}',{},920198,{},1,{},{},x'{}',0,UNIX_TIMESTAMP())",i.request_key,b.character_id,
    b.occurrence_id,b.accepted_time,i.snapshot.recipe_model,R::Hex(bytes)));
   return r.RowsAffected()==1;
  }catch(...){return false;}
 }
 bool CommitDebitAndProgress(ZoneDatabase& db,const Intent& i) override {
  try {
   Strict(db);const auto bytes=Context(i);const auto& b=i.snapshot.occurrence;const auto& p=i.snapshot.plan;
   if(R::Inspect(db,SodWeightedDeliveryContext::Identity(i.snapshot),i.request_key,bytes)!=R::State::Prepared)return false;
   Preimage(db,i);
   auto state=Query(db,fmt::format("UPDATE sod_weighted_delivery_state SET revision=revision+1,carry_pounds={},carry_expires_at={} "
    "WHERE character_id={} AND task_id=920198 AND occurrence_id={} AND activity_id=1 AND recipe_model={} "
    "AND revision={} AND carry_pounds={} AND carry_expires_at={}",p.carry_after,p.carry_expires_after,
    b.character_id,b.occurrence_id,i.snapshot.recipe_model,b.revision,b.carry_pounds,b.carry_expires_at));
   if(state.RowsAffected()!=1)return false;
   auto activity=Query(db,fmt::format("UPDATE character_activities SET donecount={},completed={} "
    "WHERE charid={} AND taskid=920198 AND activityid=1 AND donecount={} AND completed=0",
    p.progress_after,p.progress_after==10?1:0,b.character_id,b.progress));
   // A legitimate sub-ten-pound offer changes carry but not native donecount.
   if(activity.RowsAffected()>1)return false;
   Native(db,b.character_id,b.accepted_time,p.progress_after,true);
   auto receipt=Query(db,fmt::format("UPDATE sod_weighted_delivery_actions SET status=1,updated_at=UNIX_TIMESTAMP() "
    "WHERE request_key='{}' AND status=0 AND context_blob=x'{}'",i.request_key,R::Hex(bytes)));
   return receipt.RowsAffected()==1;
  }catch(...){return false;}
 }
 Ack AcknowledgeProgress(Client& c,NPC& npc,const CommittedAndSettled& proof) override {
  try {
   const auto& i=proof.Get();const auto& b=i.snapshot.occurrence;
   // The private proof means physical+semantic commit is KNOWN. Publish the
   // primitive target into the exact current native slot BEFORE even encoding
   // context or querying DB; both can fail/allocate. Core verified the current
   // personal occurrence before issuing live/reconciliation proof. If this
   // scalar identity no longer matches, the exact UnknownInventory task-save
   // guard preserves durable progress until reconnect instead of guessing.
   if(c.CharacterID()!=b.character_id || npc.GetNPCTypeID()!=i.snapshot.npc.type ||
    !zone || zone->GetZoneID()!=452 || zone->GetInstanceID() || zone->GetInstanceVersion())return Ack::ReconciliationRequired;
   auto* projected=Active(c);
   const auto projected_target=i.snapshot.plan.progress_after;
   if(!projected || uint32_t(projected->accepted_time)!=b.accepted_time || projected_target>10 ||
    projected->activity[Activity].done_count<0 ||
    (uint32_t(projected->activity[Activity].done_count)!=b.progress &&
     uint32_t(projected->activity[Activity].done_count)!=projected_target))return Ack::ReconciliationRequired;
   projected->activity[Activity].done_count=int(projected_target);
   projected->activity[Activity].activity_state=projected_target==10?ActivityCompleted:ActivityActive;
   projected->activity[Activity].updated=true;
   const auto bytes=Context(i);
   // Typed writer proof binds the ORIGINAL immutable NPC entity. A known
   // settled old hand-in may be projected after reconnect at the same bound
   // provider type with a new live entity ID; no item debit is repeated.
   const auto receipt=R::Inspect(database,SodWeightedDeliveryContext::Identity(i.snapshot),i.request_key,bytes);
   if(receipt!=R::State::Debited && receipt!=R::State::Acknowledged)return Ack::ReconciliationRequired;
   auto* info=Active(c);if(!info || uint32_t(info->accepted_time)!=b.accepted_time)return Ack::ReconciliationRequired;
   const auto target=i.snapshot.plan.progress_after;
   const auto current=info->activity[Activity].done_count;
   if(current<0 || (uint32_t(current)!=b.progress && uint32_t(current)!=target))return Ack::ReconciliationRequired;
   auto durable=Native(database,b.character_id,b.accepted_time,target,false);
   if(durable.occurrence_id!=b.occurrence_id)return Ack::ReconciliationRequired;
   // Allocation-free absolute publication precedes fallible packet/event/save
   // work. Teardown can never overwrite the already committed count with the
   // old count if native publication throws. Final real hail is still pending.
   info->activity[Activity].done_count=int(target);
   info->activity[Activity].activity_state=target==10?ActivityCompleted:ActivityActive;
   info->activity[Activity].updated=true;
   if(receipt==R::State::Acknowledged)return Ack::Settled;
   c.UpdateTaskActivity(Task,Activity,0,true); // ordinary native transition/packet/save, no second increment
   if(!Active(c) || Active(c)->activity[Activity].done_count!=int(target))return Ack::ReconciliationRequired;
   durable=Native(database,b.character_id,b.accepted_time,target,false);
   if(durable.occurrence_id!=b.occurrence_id)return Ack::ReconciliationRequired;
   auto r=Query(database,fmt::format("UPDATE sod_weighted_delivery_actions SET status=2,updated_at=UNIX_TIMESTAMP() "
    "WHERE request_key='{}' AND status=1 AND context_blob=x'{}'",i.request_key,R::Hex(bytes)));
   return r.RowsAffected()==1?Ack::Settled:Ack::ReconciliationRequired;
  }catch(...){return Ack::ReconciliationRequired;}
 }
 bool MarkReturned(ZoneDatabase& db,const ConfirmedFullReturn& proof) override {
  try {
   Strict(db);const auto& i=proof.Get();const auto bytes=Context(i);
   // Only this typed proof is exposed by the original physical writer. Its
   // common supervisor path uses the same exact helper with abort provenance.
   return R::MarkAfterVerifiedFullReturn(db,SodWeightedDeliveryContext::Identity(i.snapshot),i.request_key,bytes,0);
  }catch(...){return false;}
 }
};
}
bool ClaimsOffer(Client& c,NPC& npc) {
 if(!zone || !RuleB(SodProgression,EnableFieldOfScaleSlice) || zone->GetZoneID()!=452 ||
  zone->GetInstanceID() || zone->GetInstanceVersion() || !SodFoodBinding::Giver ||
  npc.GetNPCTypeID()!=SodFoodBinding::Giver || !c.IsTaskActive(Task))return false;
 const auto parameters=SodFoodBinding::Parameters();
 const auto offers=OriginalTradeOffer::Capture(c);
 for(const auto& item:offers)if(item.present)
  for(const auto& role:parameters.roles)if(role.item_id && item.item_id==role.item_id)return true;
 return false;
}
OriginalTradeWriter::SemanticRecipe& RecipeHooks() {static Recipe recipe;return recipe;}
}
