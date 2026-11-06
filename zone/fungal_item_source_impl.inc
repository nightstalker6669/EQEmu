// Included once by client.cpp, after native Client/NPC/task definitions.
#include "fungal_item_operation_source.h"
#include "../common/fungal_item_operation_wire.h"
namespace FungalItemSource {
namespace {
thread_local const OriginalOperation* original_personal_hail=nullptr;
thread_local const Client* original_personal_hail_client=nullptr;
thread_local bool original_personal_hail_claimed=false;
bool PurePersonalHail(uint32_t task,uint32_t activity)noexcept {
 return (task==920141&&(activity==1||activity==3||activity==5||activity==9))||
  (task==920142&&(activity==8||activity==9));
}
}
struct NativeEvents::Pending {
 std::unique_ptr<OriginalOperation> original;
 Pending* next=nullptr;
};
NativeEvents::Pending* NativeEvents::head_=nullptr;
NativeEvents::Pending* NativeEvents::tail_=nullptr;
std::shared_ptr<Lifetime> NativeEvents::zone_life_;
Zone* NativeEvents::zone_owner_=nullptr;
thread_local const CommittedOperation* NativeEvents::personal_progress_=nullptr;
thread_local const Client* NativeEvents::personal_client_=nullptr;
thread_local bool NativeEvents::personal_claimed_=false;
thread_local bool NativeEvents::personal_save_seen_=false;
bool NativeEvents::CurrentPersonal(Client& c,const AcceptedFate& b,bool initial) {
 if(!c.task_state||!b.character||b.character!=c.CharacterID()||!b.occurrence||!b.accepted||
  b.slot<0||b.slot>=MAXACTIVEQUESTS)return false;
 const auto& t=c.task_state->m_active_quests[b.slot];
 if(t.task_id!=920141||t.slot!=b.slot||uint32_t(t.accepted_time)!=b.accepted||t.was_rewarded)return false;
 const std::string activities=initial?
  " AND (SELECT COUNT(*) FROM character_activities a WHERE a.charid=c.charid AND a.taskid=c.taskid "
  "AND a.activityid BETWEEN 0 AND 9 AND a.donecount=0 AND a.completed=0)=10":"";
 auto q=database.QueryDatabase(fmt::format(
  "SELECT r.occurrence_id FROM character_task_reward_instances r JOIN character_tasks c "
  "ON c.charid=r.character_id AND c.taskid=r.task_id AND c.acceptedtime=r.accepted_time "
  "WHERE r.character_id={} AND r.task_id=920141 AND r.occurrence_id={} AND r.accepted_time={} "
  "AND c.slot={} AND c.type=2 AND c.was_rewarded=0{}",
  b.character,b.occurrence,b.accepted,b.slot,activities),false);
 return q.Success()&&q.RowCount()==1&&q.begin()[0]&&
  std::string(q.begin()[0])==std::to_string(b.occurrence);
}
void NativeEvents::Accepted(Client& c,uint64_t reserved,bool saved,const ClientTaskInformation& t) {
 if(t.task_id!=920141)return;
 c.m_fungal_fate.reset();
 AcceptedFate b{c.CharacterID(),uint32_t(std::max(t.accepted_time,0)),reserved,t.slot};
 if(saved&&CurrentPersonal(c,b,true))c.m_fungal_fate=b;
}
void NativeEvents::RestoreAccepted(Client& c) {
 c.m_fungal_fate.reset();if(!c.task_state)return;
 for(const auto& t:c.task_state->m_active_quests) {
  if(t.task_id!=920141||t.was_rewarded||t.accepted_time<=0)continue;
  auto q=database.QueryDatabase(fmt::format(
   "SELECT occurrence_id FROM character_task_reward_instances WHERE character_id={} "
   "AND task_id=920141 AND accepted_time={}",c.CharacterID(),t.accepted_time),false);
  if(!q.Success()||q.RowCount()!=1||!q.begin()[0])return;
  const auto occurrence=TaskSharedRewardIdentity::Number(q.begin()[0]);
  AcceptedFate b{c.CharacterID(),uint32_t(t.accepted_time),occurrence,t.slot};
  if(CurrentPersonal(c,b,false))c.m_fungal_fate=b;
  return;
 }
}
void NativeEvents::ReloadAccepted(Client& c){RestoreAccepted(c);}
std::unique_ptr<OriginalOperation> NativeEvents::Capture(Client& c,NPC& n,bool trade) {
 if(!zone||zone->GetZoneID()!=481||zone->GetInstanceID()!=0||zone->GetInstanceVersion()!=0||
  !c.CharacterID()||!c.GetID()||!n.GetID()||n.GetDepop()||n.IsEngaged()||!c.task_state)return {};
 const bool deagus=n.GetNPCTypeID()==481093&&n.GetSpawnPointID()==150084;
 const bool rakkirin=n.GetNPCTypeID()==481052&&n.GetSpawnPointID()==149898;
 const bool autoros=n.GetNPCTypeID()==481118&&n.GetSpawnPointID()==150223;
 const bool eulyris=n.GetNPCTypeID()==481115&&n.GetSpawnPointID()==150205;
 const bool excrug=n.GetNPCTypeID()==481109&&n.GetSpawnPointID()==150152;
 const bool orgus=n.GetNPCTypeID()==481120&&n.GetSpawnPointID()==150231;
 // Native normal-world Adeenas role; no exact static spawn was authenticated
 // by the retained source. Require a real static spawn of this original type,
 // not a transient/GM-created actor or a mission-instance namesake.
 const bool adeenas=n.GetNPCTypeID()==481071&&n.GetSpawnPointID()!=0;
 if(!deagus&&!rakkirin&&!autoros&&!eulyris&&!excrug&&!orgus&&!adeenas)return {};
 if(!trade&&(c.GetTarget()!=&n||c.IsInvisible(&n)||
  DistanceNoZ(c.GetPosition(),n.GetPosition())>RuleI(Range,Say)))return {};
 auto out=std::unique_ptr<OriginalOperation>(new OriginalOperation);
 auto& f=out->facts_;
 f.character=c.CharacterID();f.client_entity=c.GetID();f.npc_entity=n.GetID();
 f.npc_type=n.GetNPCTypeID();f.npc_spawn=n.GetSpawnPointID();
 int fate_activity=-1;
 if(!trade&&c.m_fungal_fate&&CurrentPersonal(c,*c.m_fungal_fate,false)) {
  const auto& current=c.task_state->m_active_quests[c.m_fungal_fate->slot];
  const int candidate=autoros?1:eulyris?3:excrug?5:deagus?7:adeenas?9:-1;
  if(candidate>=0&&current.activity[candidate].activity_state==ActivityActive&&
   current.activity[candidate].done_count==0)fate_activity=candidate;
 }
 if(fate_activity>=0) {
  const auto& b=*c.m_fungal_fate;
  f.scope=1;f.task=920141;f.activity=fate_activity;f.occurrence=b.occurrence;f.accepted=b.accepted;
  f.personal_slot=b.slot;
  if(fate_activity==7){f.output_item=40997;f.output_quantity=1;}
 }
 if(!f.scope&&!trade&&(adeenas||orgus)) {
  const int activity=adeenas?8:9;
  for(const auto& t:c.task_state->m_active_quests) {
   if(t.task_id!=920142||t.was_rewarded||t.accepted_time<=0||
    t.activity[activity].activity_state!=ActivityActive||t.activity[activity].done_count!=0)continue;
   auto q=database.QueryDatabase(fmt::format(
    "SELECT r.occurrence_id FROM character_task_reward_instances r JOIN character_tasks ct "
    "ON ct.charid=r.character_id AND ct.taskid=r.task_id AND ct.acceptedtime=r.accepted_time "
    "WHERE r.character_id={} AND r.task_id=920142 AND r.accepted_time={} "
    "AND ct.type=2 AND ct.slot={} AND ct.was_rewarded=0",c.CharacterID(),t.accepted_time,t.slot),false);
   if(!q.Success()||q.RowCount()!=1||!q.begin()[0])return {};
   const auto occurrence=TaskSharedRewardIdentity::Number(q.begin()[0]);if(!occurrence)return {};
   f.scope=1;f.task=920142;f.activity=activity;f.occurrence=occurrence;
   f.accepted=uint32_t(t.accepted_time);f.personal_slot=t.slot;break;
  }
 }
 if(!f.scope) {
  auto& t=c.task_state->m_active_shared_task;
  int selected=trade&&deagus?5:-1;
  if(!trade&&rakkirin){for(int a:{0,10})if(t.activity[a].activity_state==ActivityActive&&t.activity[a].done_count==0){selected=a;break;}}
  if(!trade&&deagus){for(int a:{1,6})if(t.activity[a].activity_state==ActivityActive&&t.activity[a].done_count==0){selected=a;break;}}
  if(selected<0)return {};const uint32_t activity=uint32_t(selected);
  if(t.task_id!=920143||t.was_rewarded||t.accepted_time<=0||
   t.activity[activity].activity_state!=ActivityActive||t.activity[activity].done_count!=0)return {};
  const auto run=TaskSharedRewardIdentity::ReadRun(database,c.CharacterID(),920143);
  if(!run||c.GetSharedTaskId()<=0||run->id!=uint64_t(c.GetSharedTaskId())||
   run->accepted_time!=uint32_t(t.accepted_time))return {};
  auto q=database.QueryDatabase(fmt::format(
   "SELECT s.id FROM shared_tasks s JOIN shared_task_members m ON m.shared_task_id=s.id "
   "WHERE s.id={} AND s.task_id=920143 AND m.character_id={} AND s.completion_time IS NULL "
   "AND (s.expire_time IS NULL OR s.expire_time>NOW()) AND UNIX_TIMESTAMP(s.accepted_time)={}",
   run->id,c.CharacterID(),run->accepted_time),false);
  if(!q.Success()||q.RowCount()!=1)return {};
  if(activity==10) {
   auto deliveries=database.QueryDatabase(fmt::format(
    "SELECT activity_id,done_count,completed_time IS NOT NULL FROM shared_task_activity_state "
    "WHERE shared_task_id={} AND activity_id BETWEEN 7 AND 9 ORDER BY activity_id",run->id),false);
   if(!deliveries.Success()||deliveries.RowCount()!=3)return {};
   unsigned i=0;for(auto row:deliveries) {
    const uint32_t goals[]={4,3,2};
    if(TaskSharedRewardIdentity::Number(row[0])!=7+i||
     TaskSharedRewardIdentity::Number(row[1])!=goals[i]||!row[2]||std::string(row[2])!="1"||
     t.activity[7+i].activity_state!=ActivityCompleted||t.activity[7+i].done_count!=int(goals[i]))return {};
    ++i;
   }
  }
  f.scope=2;f.task=920143;f.activity=activity;f.occurrence=run->id;f.accepted=run->accepted_time;
  if(trade){f.output_item=43586;f.output_quantity=2;}
  else if(activity==10){f.output_item=43587;f.output_quantity=1;}
  if(trade){f.input_item=43585;f.input_quantity=2;}
 }
 const auto* task=TaskManager::Instance()->GetTaskData(f.task);
 if(!task||task->type!=(f.scope==1?TaskType::Quest:TaskType::Shared)||
  task->reward_method!=METHODSELECT||!task->has_reward_selection||
  task->activity_count!=(f.task==920142?10:f.scope==1?10:12)||
  task->activity_information[f.activity].goal_count!=(f.scope==2&&f.activity==5?2:1)||
  task->activity_information[f.activity].activity_type!=(trade?TaskActivityType::Deliver:TaskActivityType::SpeakWith))return {};
 static uint64_t sequence=0;if(sequence==UINT64_MAX)return {};f.source_sequence=++sequence;
 if(!c.m_fungal_source_life)c.m_fungal_source_life=std::shared_ptr<Lifetime>(new Lifetime);
 if(!n.m_fungal_source_life)n.m_fungal_source_life=std::shared_ptr<Lifetime>(new Lifetime);
 if(!zone_life_||zone_owner_!=zone) {
  if(zone_life_)zone_life_->active_=false;
  zone_life_=std::shared_ptr<Lifetime>(new Lifetime);zone_owner_=zone;
 }
 if(!c.m_fungal_source_life->active_||!n.m_fungal_source_life->active_||!zone_life_->active_)return {};
 out->client_=&c;out->npc_=&n;out->zone_=zone;out->trade_=trade;
 out->client_life_=c.m_fungal_source_life;out->npc_life_=n.m_fungal_source_life;out->zone_life_=zone_life_;
 if(trade) {
  if(!c.trade||c.trade->state!=Trading||c.trade->With()!=&n)return {};
  out->roots_.reserve(4);
  for(int16_t slot=3000;slot<=3003;++slot) {
   auto* item=c.GetInv().GetItem(slot);if(!item)continue;
   out->roots_.push_back({slot,item,item->GetID(),item->GetSerialNumber(),item->GetCharges()});
  }
  out->copper_=c.trade->cp;out->silver_=c.trade->sp;out->gold_=c.trade->gp;out->platinum_=c.trade->pp;
  out->finish_=std::make_shared<EQApplicationPacket>(OP_FinishTrade,0);
 }
 if(f.scope==2) {
  out->progress_=std::make_shared<ServerPacket>(ServerOP_SharedTaskUpdate,
   sizeof(ServerSharedTaskActivityUpdate_Struct)+sizeof(FungalItemWire::Trailer));
  auto* p=reinterpret_cast<ServerSharedTaskActivityUpdate_Struct*>(out->progress_->pBuffer);
  p->source_character_id=f.character;p->task_id=f.task;p->activity_id=f.activity;
  p->done_count=f.scope==2&&f.activity==5?2:1;p->ignore_quest_update=false;
 }
 return out;
}
bool NativeEvents::Revalidate(Client& c,const OriginalOperation& o) {
 if(!o.zone_life_||!o.zone_life_->active_||zone!=o.zone_||zone_owner_!=zone||
  !o.client_life_||!o.client_life_->active_||!o.npc_life_||!o.npc_life_->active_||
  entity_list.GetClientByCharID(o.facts_.character)!=&c||&c!=o.client_||
  c.m_fungal_source_life!=o.client_life_||c.GetID()!=o.facts_.client_entity)return false;
 auto* n=entity_list.GetNPCByID(o.facts_.npc_entity);
 if(n!=o.npc_||!n||n->m_fungal_source_life!=o.npc_life_)return false;
 // Capture freshly observes stage/run/member, but does not create entitlement.
 auto now=Capture(c,*n,o.trade_);if(!now)return false;
 const auto& f=now->facts_;const auto& old=o.facts_;
 if(f.scope!=old.scope||f.task!=old.task||f.activity!=old.activity||f.occurrence!=old.occurrence||
  f.accepted!=old.accepted||f.personal_slot!=old.personal_slot)return false;
 if(o.trade_) {
  if(now->roots_.size()!=o.roots_.size()||now->copper_!=o.copper_||now->silver_!=o.silver_||
   now->gold_!=o.gold_||now->platinum_!=o.platinum_)return false;
  for(size_t i=0;i<o.roots_.size();++i) {
   const auto& a=o.roots_[i];const auto& b=now->roots_[i];
   if(a.slot!=b.slot||a.original!=b.original||a.item!=b.item||a.serial!=b.serial||a.charges!=b.charges)return false;
  }
 }
 return true;
}
void NativeEvents::CaptureSay(Client& c,const char* message) {
 if(!message||!zone||zone->GetZoneID()!=481||zone->GetInstanceID()!=0)return;
 try {
  if(!Strings::BeginsWith(Strings::ToLower(message),"hail"))return;
  auto* target=c.GetTarget();if(!target||!target->IsNPC())return;
  auto o=Capture(c,*target->CastToNPC(),false);if(!o)return;
  auto p=std::unique_ptr<Pending>(new Pending{std::move(o),nullptr});
  if(tail_)tail_->next=p.get();else head_=p.get();tail_=p.release();
 }catch(const std::bad_alloc&){} // Original native say remains untouched; no spend occurred.
}
bool NativeEvents::OriginalTrade(Client& c,NPC& n)noexcept {
 if(!zone||zone->GetZoneID()!=481||zone->GetInstanceID()||n.GetNPCTypeID()!=481093||
  n.GetSpawnPointID()!=150084)return false;
 bool selected=c.task_state&&c.task_state->m_active_shared_task.task_id==920143&&
  c.task_state->m_active_shared_task.activity[5].activity_state==ActivityActive;
 for(int16_t slot=3000;slot<=3003;++slot) {
  const auto* item=c.GetInv().GetItem(slot);if(item&&item->GetID()==43585)selected=true;
 }
 if(!selected)return false;
 try {
  auto original=Capture(c,n,true);if(!original)return true;
  auto result=FungalInventory::NativeCustody::Execute(c,std::move(original));
  if(result)Publish(c,std::move(result));
 }catch(...){} // selected original roots/cash never fall through to generic popping
 return true;
}
void NativeEvents::Publish(Client& c,std::unique_ptr<CommittedOperation> receipt) {
 if(!receipt||receipt->published_||!receipt->outcome_)return;
 if(!receipt->client_life_||!receipt->client_life_->active_||
  !receipt->npc_life_||!receipt->npc_life_->active_||!receipt->zone_life_||!receipt->zone_life_->active_||
  zone!=receipt->original_zone_||&c!=receipt->original_client_||
  entity_list.GetClientByCharID(receipt->facts_.character)!=receipt->original_client_)return;
 receipt->published_=true;const auto f=receipt->facts_;
 if(f.input_item==43585) {
  if(!c.trade||c.trade->state!=Trading||c.trade->With()!=receipt->original_npc_)return;
  bool remaining=c.trade->cp||c.trade->sp||c.trade->gp||c.trade->pp;
  for(int16_t slot=3000;slot<=3003;++slot)if(c.GetInv().GetItem(slot))remaining=true;
  // Metadata is finalized before mutable native progress callbacks. An offer
  // with remaining excess/other property stays open for ordinary cancellation;
  // it is never reset or passed to the destructive generic NPC hand-in path.
  if(!remaining) {
   if(!receipt->finish_)return;
   c.QueuePacket(receipt->finish_.get());c.trade->Reset();
  }
 }
 if(f.scope==2) {
  if(!receipt->progress_)return;
  FungalItemWire::Trailer t;t.run=f.occurrence;t.outcome=receipt->outcome_;
  t.accepted=f.accepted;t.recipient=f.character;
  FungalItemWire::Write(receipt->progress_->pBuffer,sizeof(ServerSharedTaskActivityUpdate_Struct),t);
  worldserver.SendPacket(receipt->progress_.get());return;
 }
 if(f.scope!=1||f.task!=920141||f.activity!=7||f.character!=c.CharacterID())return;
 const auto* previous=personal_progress_;const auto* previous_client=personal_client_;
 const bool previous_claimed=personal_claimed_,previous_saved=personal_save_seen_;
 personal_progress_=receipt.get();personal_client_=&c;personal_claimed_=personal_save_seen_=false;
 struct Restore {const CommittedOperation*& field;const CommittedOperation* old;
  const Client*& client;const Client* old_client;bool& claimed;bool old_claimed;bool& saved;bool old_saved;
  ~Restore(){field=old;client=old_client;claimed=old_claimed;saved=old_saved;}}
  restore{personal_progress_,previous,personal_client_,previous_client,personal_claimed_,previous_claimed,
   personal_save_seen_,previous_saved};
 // Last actor operation: native progress/save/callbacks may destroy the actor.
 c.UpdateTaskActivity(920141,7,1);
}
bool NativeEvents::AllowPersonalProgress(const Client* c,uint32_t task,uint32_t activity) {
 if(PurePersonalHail(task,activity)) {
  const auto* proof=original_personal_hail;
  if(!proof||!c||c!=original_personal_hail_client||c!=proof->client_||
   original_personal_hail_claimed||proof->facts_.scope!=1||proof->facts_.task!=task||
   proof->facts_.activity!=activity||proof->facts_.input_item||proof->facts_.output_item||
   !proof->client_life_||!proof->client_life_->active_||
   !proof->npc_life_||!proof->npc_life_->active_||!proof->zone_life_||!proof->zone_life_->active_||
   !Revalidate(*proof->client_,*proof))return false;
  original_personal_hail_claimed=true;return true;
 }
 if(task!=920141||activity!=7)return true;
 if(!personal_progress_||!personal_progress_->published_||!c||c!=personal_client_||personal_claimed_)return false;
 personal_claimed_=true;return true;
}
void NativeEvents::ActualPersonalSave(const Client* c,uint32_t task,uint32_t activity,bool saved) {
 if(task!=920141||activity!=7||!personal_progress_||c!=personal_client_||!personal_claimed_||personal_save_seen_)return;
 personal_save_seen_=true;
 // Exact pointer comparison only; no post-callback Client dereference.
 FungalInventory::NativeCustody::PersonalStageSaved(database,*personal_progress_,saved);
}
bool NativeEvents::AcceptWorldEcho(Client& c,const void* bytes,uint32_t size,
 uint32_t task,uint32_t activity,uint32_t count) {
 if(!FungalItemWire::Correlated(task,activity))return true;
 FungalItemWire::Trailer t;
 const bool pure=FungalItemWire::PureHail(task,activity);
 const bool decoded=pure?FungalItemWire::ReadHeader(bytes,size,sizeof(ServerSharedTaskActivityUpdate_Struct),t):
  FungalItemWire::Read(bytes,size,sizeof(ServerSharedTaskActivityUpdate_Struct),t);
 if(!decoded||(pure&&t.outcome)||
  !c.task_state||c.task_state->m_active_shared_task.task_id!=920143||c.GetSharedTaskId()<=0||
  uint64_t(c.GetSharedTaskId())!=t.run||uint32_t(c.task_state->m_active_shared_task.accepted_time)!=t.accepted||
  count!=(activity==5?2u:1u))return false;
 const auto run=TaskSharedRewardIdentity::ReadRun(database,c.CharacterID(),920143);
 if(!run||run->id!=t.run||run->accepted_time!=t.accepted)return false;
 if(!pure)return FungalInventory::NativeCustody::KnownWorldStage(database,t.run,t.outcome,t.accepted,t.recipient,activity,count);
 // This path handles an actual trusted World echo, never creates an item or
 // a progress request from history. Only the World known-COMMIT scope stamps it.
 auto q=database.QueryDatabase(fmt::format(
  "SELECT done_count,completed_time IS NOT NULL FROM shared_task_activity_state "
  "WHERE shared_task_id={} AND activity_id={}",t.run,activity),false);
 return q.Success()&&q.RowCount()==1&&TaskSharedRewardIdentity::Number(q.begin()[0])==1&&
  q.begin()[1]&&std::string(q.begin()[1])=="1";
}
void NativeEvents::Drain() {
 auto* pending=head_;head_=tail_=nullptr;
 while(pending) {
  std::unique_ptr<Pending> event(pending);pending=pending->next;
  auto& o=*event->original;
  if(!o.client_life_||!o.client_life_->active_||!o.zone_life_||!o.zone_life_->active_)continue;
  auto* c=entity_list.GetClientByCharID(o.facts_.character);
  if(!c||c!=o.client_||!Revalidate(*c,o))continue;
  if(!o.facts_.input_item&&!o.facts_.output_item) {
   const auto f=o.facts_;
   if(f.scope==1&&PurePersonalHail(f.task,f.activity)) {
    const auto* old=original_personal_hail;const auto* old_client=original_personal_hail_client;
    const bool old_claimed=original_personal_hail_claimed;
    original_personal_hail=&o;original_personal_hail_client=c;original_personal_hail_claimed=false;
    struct RestorePure {
     const OriginalOperation*& operation;const OriginalOperation* old;
     const Client*& client;const Client* old_client;bool& claimed;bool old_claimed;
     ~RestorePure(){operation=old;client=old_client;claimed=old_claimed;}
    } restore{original_personal_hail,old,original_personal_hail_client,old_client,
     original_personal_hail_claimed,old_claimed};
    c->UpdateTaskActivity(f.task,f.activity,1);continue; // No actor access after callbacks.
   }
   if(f.scope==2&&FungalItemWire::PureHail(f.task,f.activity)&&o.progress_) {
    FungalItemWire::Trailer t;t.run=f.occurrence;t.accepted=f.accepted;t.recipient=f.character;t.outcome=0;
    FungalItemWire::Write(o.progress_->pBuffer,sizeof(ServerSharedTaskActivityUpdate_Struct),t);
    worldserver.SendPacket(o.progress_.get());
   }
   continue;
  }
  auto committed=FungalInventory::NativeCustody::Execute(*c,std::move(event->original));
  if(committed)Publish(*c,std::move(committed));
  // No actor access after native progress/callback publication.
 }
}
void NativeEvents::Discard()noexcept {
 if(zone_life_)zone_life_->active_=false;zone_life_.reset();zone_owner_=nullptr;
 auto* p=head_;head_=tail_=nullptr;while(p){auto* next=p->next;delete p;p=next;}
}
void NativeEvents::Revoke(Client& c)noexcept {if(c.m_fungal_source_life)c.m_fungal_source_life->active_=false;}
void NativeEvents::Revoke(NPC& n)noexcept {if(n.m_fungal_source_life)n.m_fungal_source_life->active_=false;}
} // namespace FungalItemSource
