// Included after the native SharedTaskManager implementation. No new World
// opcode: Core's committed original physical intent is the durable outbox.
namespace SodOriginalTransformWorld {
inline bool Definition(const SharedTask& run) {
 const auto& task=run.GetTaskData();
 if(task.id!=920197||task.type!=1||task.title!="Drake Scourge")return false;
 std::array<bool,4> seen{};const std::array<int,4> steps{{0,0,1,2}};
 const auto& activities=run.GetTaskActivityData();if(activities.size()!=4)return false;
 for(const auto& a:activities){
  if(a.taskid!=920197||a.activityid>3||seen[a.activityid]||a.optional||a.step!=steps[a.activityid]||a.goalcount!=(a.activityid==0?4:1))return false;
  seen[a.activityid]=true;
  if(a.activityid==2){if(a.activitytype!=uint8_t(TaskActivityType::None))return false;}
  else if(a.activitytype!=uint8_t(TaskActivityType::Deliver)||a.npc_match_list!="452108"||a.item_id_list!=(a.activityid==0?"88325":"88326"))return false;
 }
 return true;
}
inline bool ContentAllowed(Database& content) {
 auto q=S::Q(content,"SELECT enabled,type,title FROM tasks WHERE id=920197");
 if(q.RowCount()>1)throw std::runtime_error("Original World task identity ambiguous");
 if(!q.RowCount())return false;
 auto r=q.begin();if(!r[2])throw std::runtime_error("Original World task title absent");
 return RuleB(SodProgression,EnableFieldOfScaleSlice)&&RuleB(TaskSystem,RecordCompletedTasks)&&S::N(r[0])==1&&S::N(r[1])==1&&std::string(r[2])=="Drake Scourge";
}
inline std::vector<SharedTaskActivityStateEntry> Canonical(Database& db,const SharedTask& run,bool lock) {
 if(!Definition(run))throw std::runtime_error("Original World native definition differs");
 auto out=run.GetActivityState();if(out.size()!=4)throw std::runtime_error("Original World cached activity set differs");
 auto q=S::Q(db,fmt::format("SELECT activity_id,done_count,UNIX_TIMESTAMP(updated_time),UNIX_TIMESTAMP(completed_time) "
  "FROM shared_task_activity_state WHERE shared_task_id={} ORDER BY activity_id{}",run.GetDbSharedTask().id,lock?" FOR UPDATE":""));
 if(q.RowCount()!=4)throw std::runtime_error("Original World canonical activity set absent");
 std::array<bool,4> seen{};
 for(auto row:q){
  const auto id=S::N(row[0]),done=S::N(row[1]),updated=row[2]?S::N(row[2]):0,completed=row[3]?S::N(row[3]):0;
  if(id>3||seen[id]||done>(id==0?4:1)||updated>UINT32_MAX||completed>UINT32_MAX||((done==(id==0?4:1))!=(completed>0)))
   throw std::runtime_error("Original World canonical activity evidence differs");
  seen[id]=true;
  auto a=std::find_if(out.begin(),out.end(),[&](const auto& v){return v.activity_id==id;});
  if(a==out.end()||a->max_done_count!=(id==0?4u:1u))throw std::runtime_error("Original World activity template differs");
  a->done_count=uint32_t(done);a->updated_time=uint32_t(updated);a->completed_time=uint32_t(completed);
  a->activity_state=completed?ActivityState::ActivityCompleted:ActivityState::ActivityHidden;
 }
 return out;
}
inline SharedTaskActivityStateEntry& Activity(std::vector<SharedTaskActivityStateEntry>& states,unsigned id) {
 auto a=std::find_if(states.begin(),states.end(),[&](const auto& v){return v.activity_id==id;});
 if(a==states.end())throw std::runtime_error("Original World selected activity absent");return *a;
}
}

void SharedTaskManager::ProcessOriginalTransforms()
{
 namespace W=SodOriginalTransformWorld;namespace S=SodOriginalTransform;namespace X=EQEmu::SharedCore::OriginalTradeTransform;
 // Check only this optional schema. A stock/default-OFF installation performs
 // no request polling after a successful absent-schema observation.
 try{
  if(!m_original_transform_schema_ready||(!*m_original_transform_schema_ready&&RuleB(SodProgression,EnableFieldOfScaleSlice))){
   auto q=S::Q(*m_database,"SELECT (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
    "AND table_name='sod_original_delivery_decisions' AND engine='InnoDB')=1 AND "
    "(SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='kithicor_poison_custody' "
    "AND column_name IN('recipe_kind','recipe_context','abort_only'))=3");
   if(q.RowCount()!=1)return;m_original_transform_schema_ready=S::N(q.begin()[0])==1;
  }
  if(!*m_original_transform_schema_ready)return;
 }catch(...){return;}

 // Known decisions are excluded, so an offline actor awaiting physical
 // recovery cannot starve newly committed requests behind it.
 try{
  auto pending=S::Q(*m_database,"SELECT j.request_key,HEX(j.recipe_context) FROM kithicor_poison_custody j "
   "LEFT JOIN sod_original_delivery_decisions d ON d.request_key=j.request_key "
   "WHERE j.recipe_kind=3 AND j.shared_task_id>0 AND j.physical=0 AND j.settled=0 AND j.abort_only=0 "
   "AND d.request_key IS NULL ORDER BY j.request_key LIMIT 64");
  for(auto row:pending){
   bool transaction=false;
   try{
    if(!row[0])throw std::runtime_error("Original World request key absent");
    const std::string key(row[0]);const auto c=X::Decode(S::Unhex(row[1]));
    if(!c.shared||!S::Key(key)||c.model!=S::Model)throw std::runtime_error("Original World outbox context differs");
    const bool enabled=W::ContentAllowed(*m_content_database);
    if(!m_database->TransactionBeginStrict().Success())continue;transaction=true;
    // The SAME common writer proves actor-first exact lease/current physical
    // preimage, including inventory/currency/evolving custody. Do not recreate
    // its blob codec or infer original property from a semantic identity row.
    EQEmu::SharedCore::OriginalTradeCustody::ValidateWorldOutboxUnderTransaction(*m_database,key,c);
    const auto physical=S::ReadPhysical(*m_database,key,c,true);
    if(physical.outcome||physical.settled||physical.abort_only)throw std::runtime_error("Original World request no longer pending");
    auto known=S::ReadFinal(*m_database,key,c,true);
    std::vector<SharedTaskActivityStateEntry> next;
    SharedTask* run=nullptr;
    if(known.state==X::DecisionState::Pending){
     X::Decision decision;decision.state=X::DecisionState::Known;unsigned reason=2;
     if(S::ActiveShared(*m_database,c,true)){
      run=FindSharedTaskById(int64_t(c.occurrence));
      if(!run||run->FindMemberFromCharacterID(c.character).character_id!=c.character)
       throw std::runtime_error("Original World native cache unavailable");
      next=W::Canonical(*m_database,*run,true);
      auto& tail=W::Activity(next,0);auto& skull=W::Activity(next,1);
      decision.counters_present=true;
      decision.tail_before=decision.tail_target=tail.done_count;
      decision.primary_before=decision.primary_target=skull.done_count;
      const auto active=Tasks::GetActiveElements(next,next,next.size());
      const auto allowed=[&](unsigned id){return std::find(active.active.begin(),active.active.end(),id)!=active.active.end();};
      reason=enabled?(allowed(0)||allowed(1)?1:4):4;
      if(enabled){
       for(unsigned i=0;i<4;++i){
        if(!c.requested[i]||!allowed(c.activities[i]))continue;
        auto& target=c.activities[i]==0?tail:skull;
        const auto accepted=std::min(c.requested[i],target.max_done_count-target.done_count);
        decision.accepted[i]=accepted;target.done_count+=accepted;
        if(accepted){target.updated_time=uint32_t(std::time(nullptr));
         if(target.done_count==target.max_done_count){target.completed_time=target.updated_time;target.activity_state=ActivityState::ActivityCompleted;}}
       }
       decision.tail_target=tail.done_count;decision.primary_target=skull.done_count;
      }
     }
     if(!X::Valid(c,decision))throw std::runtime_error("Original World accepted-root attribution differs");
     const bool accepted=std::any_of(decision.accepted.begin(),decision.accepted.end(),[](uint32_t n){return n!=0;});
     if(accepted){
      W::Save selected(*m_database,key,c,decision);
      SaveSharedTaskActivityState(int64_t(c.occurrence),next);
      if(!selected.Saved())throw std::runtime_error("Original World actual native saver was not reached");
     }else S::InsertFinal(*m_database,key,c,decision,reason);
    }
    if(m_database->TransactionStrictFailed())throw std::runtime_error("Original World selected transaction failed");
    const bool committed=m_database->TransactionCommitStrict().Success();transaction=false;
    if(!committed)continue; // Unknown commit: SAME request is read back, never reissued as a new debit/delta.
    if(run&&!next.empty())run->SetSharedTaskActivityState(next);
    // No input/output publication or member count broadcast before physical
    // settlement. Core's pending-only online poll/recovery reads this exact key.
   }catch(...){if(transaction)m_database->TransactionRollbackStrict();}
  }
 }catch(...){return;}

 // Publish current acknowledged canonical values, never an old receipt target
 // over a newer member's contribution. Absolute native broadcasts are safe to
 // repeat after a World restart; normal quest callbacks are suppressed here.
 for(auto& run:m_shared_tasks){
  if(run.GetTaskData().id!=920197)continue;
  try{
   const auto id=uint64_t(run.GetDbSharedTask().id);
   if(S::RunPending(*m_database,id))continue;
   auto current=W::Canonical(*m_database,run,false);
   const std::array<uint32_t,2> counts{{W::Activity(current,0).done_count,W::Activity(current,1).done_count}};
   auto prior=m_original_transform_published.find(id);
   run.SetSharedTaskActivityState(current);
   if(prior==m_original_transform_published.end()||prior->second!=counts){
    SendActivityUpdate(&run,0,counts[0],true);SendActivityUpdate(&run,1,counts[1],true);
    m_original_transform_published[id]=counts;
   }
  }catch(...){/* Unknown evidence retains the run hold/projection fence. */}
 }
 for(auto it=m_original_transform_published.begin();it!=m_original_transform_published.end();)
  if(!FindSharedTaskById(int64_t(it->first)))it=m_original_transform_published.erase(it);else ++it;
}

void SharedTaskManager::ProcessOriginalMessageAdmissions() {
 namespace M=SodMessageAdmission;
 try {
  static bool installed=false;
  if(!installed&&!RuleB(SodProgression,EnableFieldOfScaleSlice))return;
  if(!installed){auto q=M::S::Q(*m_database,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='sod_original_message_selectors' AND engine='InnoDB'");if(q.RowCount()!=1||M::S::N(q.begin()[0])!=1)return;installed=true;}
  auto q=M::S::Q(*m_database,fmt::format("SELECT s.request_key,HEX(s.native_roster_proof),s.npc_entity_id FROM sod_original_message_selectors s LEFT JOIN sod_original_delivery_decisions d ON d.request_key=s.request_key WHERE s.state=1 AND d.request_key IS NULL AND s.request_key>'{}' ORDER BY s.request_key LIMIT 64",m_original_message_cursor));
  if(!q.RowCount()){m_original_message_cursor.clear();return;}
  for(auto row:q){
   if(!row[0])continue;m_original_message_cursor=row[0];
   const auto bytes=M::S::Unhex(row[1]);if(bytes.size()!=sizeof(SodAdmission::Proof))continue;
   SodAdmission::Proof p;std::memcpy(&p,bytes.data(),sizeof(p));
   const auto entity=M::S::N(row[2]);if(!M::Task(p.task)||!SodAdmission::Shape(p)||entity>UINT32_MAX)continue;
   AttemptSharedTaskCreation(p.task,p.requester,uint32_t(entity),nullptr,&p);
  }
 }catch(...){/* Unknown SQL/outcome stays held; next native poll uses same key. */}
}
