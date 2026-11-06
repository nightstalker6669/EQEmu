// Included ONLY by fire_native_item_custody.cpp. Finite Fire170 ledger/issuer,
// not a public persistence service. No schema creation or automatic recovery.
namespace FireRunActionSource {
struct RunLatchSnapshot {
 RunActionKind kind{};
 RunActionBoundary boundary=RunActionBoundary::CommittedFirstRunAction;
 uint64_t run=0,row=0;
 uint32_t creator=0;
 uint64_t occurrence=0,sequence=0;
 uint8_t state=0,role=0,mask=0;
 bool debit=false;
};
struct PublishedRunActionReceipt::State {
 const Client* owner=nullptr;
 bool taken=false;
 PublishedRunActionView view{};
 std::unique_ptr<OriginalDoorUse> door_original;
 std::unique_ptr<OriginalHelperUse> helper_original;
 std::unique_ptr<PreparedRunDoorPlan> door_plan;
 std::unique_ptr<PreparedRunHelperPlan> helper_plan;
};
PublishedRunActionReceipt::PublishedRunActionReceipt(std::unique_ptr<State> s) noexcept:state_(std::move(s)){}
PublishedRunActionReceipt::~PublishedRunActionReceipt()=default;
PublishedRunActionReceipt::PublishedRunActionReceipt(PublishedRunActionReceipt&& o) noexcept:state_(std::move(o.state_)){}
std::optional<PublishedRunActionView> PublishedRunActionReceipt::Take(const Client& actual) noexcept {
 if(!state_||state_->taken||state_->owner!=&actual)return {};
 state_->taken=true;return state_->view;
}
namespace {
uint64_t ActionNumber(const char* text) {
 if(!text||!*text)throw std::runtime_error("Fire action null number");
 uint64_t value=0;const auto end=text+std::strlen(text);
 const auto p=std::from_chars(text,end,value);
 if(p.ec!=std::errc{}||p.ptr!=end)throw std::runtime_error("Fire action invalid number");
 return value;
}
MySQLRequestResult ActionQuery(Database& db,const std::string& sql) {
 if(db.GetStatus()!=DBcore::Connected)throw std::runtime_error("Fire action disconnected");
 auto q=db.QueryDatabase(sql,false);
 if(!q.Success())throw std::runtime_error("Fire action query failed");
 return q;
}
void ActionSession(Database& db,uint64_t connection) {
 auto q=ActionQuery(db,"SELECT CONNECTION_ID(),@@in_transaction");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=connection||ActionNumber(q.begin()[1])!=1||db.TransactionStrictFailed())
  throw std::runtime_error("Fire action strict session differs");
}
void ActionFoundation(Database& db) {
 auto q=ActionQuery(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
  "AND engine='InnoDB' AND table_name IN ('rof2_fire_run_actions','shared_tasks','shared_task_members',"
  "'character_shared_task_reward_instances','character_task_reward_instances','character_tasks',"
  "'shared_task_dynamic_zones','dynamic_zones','dynamic_zone_members','instance_list','instance_list_player','shared_task_activity_state')");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=12)throw std::runtime_error("Fire action foundation differs");
 q=ActionQuery(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='rof2_fire_run_actions'");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=29)throw std::runtime_error("Fire action schema differs");
 q=ActionQuery(db,"SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() "
  "AND table_name='rof2_fire_run_actions' AND index_name='one_action_per_run' AND non_unique=0 "
  "AND ((seq_in_index=1 AND column_name='shared_task_id') OR (seq_in_index=2 AND column_name='action_kind'))");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=2)throw std::runtime_error("Fire action unique run key differs");
}
bool ValidMission(const MissionRunFacts& m) {
 return m.run&&m.local_occurrence&&m.character&&m.accepted_time&&m.zone==490&&m.version==1&&m.instance&&m.dynamic_zone;
}
uint8_t RoleBit(RunActionKind kind,GateRole role) {
 if(kind==RunActionKind::WorshipPair) {
  if(role==GateRole::WorshipLeft)return 1;if(role==GateRole::WorshipRight)return 2;
 }else if(kind==RunActionKind::HelperActivation) {
  if(role==GateRole::KeymasterGarden)return 1;if(role==GateRole::KeymasterPool)return 2;
 }else if((kind==RunActionKind::InnerVault&&role==GateRole::InnerVault)||
  (kind==RunActionKind::GuardianVaultKey&&role==GateRole::GuardianVault))return 1;
 return 0;
}
std::unique_ptr<RunLatchSnapshot> PreviewLatch(RunActionKind kind,const MissionRunFacts& m) {
 if(!ValidMission(m))return {};
 auto q=ActionQuery(database,fmt::format(
  "SELECT id,creator_character_id,creator_local_occurrence,native_sequence,state,source_role,published_role_mask "
  "FROM rof2_fire_run_actions WHERE shared_task_id={} AND action_kind={}",m.run,unsigned(kind)));
 if(q.RowCount()>1)return {};
 auto result=std::make_unique<RunLatchSnapshot>();result->kind=kind;result->run=m.run;
 result->debit=kind==RunActionKind::InnerVault||kind==RunActionKind::WorshipPair||kind==RunActionKind::HelperActivation;
 if(!q.RowCount())return result;
 auto r=q.begin();result->row=ActionNumber(r[0]);result->creator=ActionNumber(r[1]);
 result->occurrence=ActionNumber(r[2]);result->sequence=ActionNumber(r[3]);result->state=ActionNumber(r[4]);
 result->role=ActionNumber(r[5]);result->mask=ActionNumber(r[6]);
 // A helper is unique for the whole run, including lost/unknown attempts.
 // Pending/terminal-hold rows never authorize retry or a second material debit.
 if(kind==RunActionKind::HelperActivation||result->state!=1||!result->mask)return {};
 result->boundary=RunActionBoundary::ExistingPublishedRunLatch;result->debit=false;
 return result;
}
void LockCurrentRun(Database& db,const MissionRunFacts& m) {
 if(!ValidMission(m))throw std::runtime_error("Fire action invalid native mission");
 auto q=ActionQuery(db,fmt::format(
  "SELECT h.occurrence_id,d.id FROM shared_tasks s "
  "JOIN shared_task_members member ON member.shared_task_id=s.id AND member.character_id={} "
  "JOIN character_shared_task_reward_instances h ON h.shared_task_id=s.id AND h.character_id=member.character_id AND h.task_id=s.task_id "
  "JOIN character_task_reward_instances r ON r.character_id=h.character_id AND r.task_id=h.task_id AND r.occurrence_id=h.occurrence_id "
  "JOIN character_tasks ct ON ct.charid=r.character_id AND ct.taskid=r.task_id AND ct.acceptedtime=r.accepted_time "
  "JOIN shared_task_dynamic_zones link ON link.shared_task_id=s.id "
  "JOIN dynamic_zones d ON d.id=link.dynamic_zone_id "
  "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=d.id AND dm.character_id=member.character_id "
  "JOIN instance_list i ON i.id=d.instance_id JOIN instance_list_player ip ON ip.id=i.id AND ip.charid=member.character_id "
  "WHERE s.id={} AND s.task_id=920170 AND s.accepted_time=FROM_UNIXTIME({}) "
  "AND s.completion_time IS NULL AND (s.expire_time IS NULL OR s.expire_time>NOW()) "
  "AND h.occurrence_id={} AND r.accepted_time={} AND ct.type={} AND ct.was_rewarded=0 "
  "AND d.id={} AND d.instance_id={} AND d.type={} AND i.zone=490 AND i.version=1 "
  "AND (i.never_expires=1 OR i.expire_at>UNIX_TIMESTAMP()) "
  "AND (SELECT COUNT(*) FROM shared_task_dynamic_zones WHERE shared_task_id=s.id)=1 FOR UPDATE",
  m.character,m.run,m.accepted_time,m.local_occurrence,m.accepted_time,static_cast<int>(TaskType::Shared),
  m.dynamic_zone,m.instance,static_cast<int>(DynamicZoneType::Mission)));
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=m.local_occurrence||ActionNumber(q.begin()[1])!=m.dynamic_zone)
  throw std::runtime_error("Fire action native run/member changed");
}
void FirstStage(Database& db,uint64_t run,RunActionKind kind) {
 int activity=kind==RunActionKind::InnerVault?3:kind==RunActionKind::WorshipPair?7:kind==RunActionKind::HelperActivation?14:-1;
 if(activity>=0) {
  auto q=ActionQuery(db,fmt::format("SELECT done_count,UNIX_TIMESTAMP(completed_time) FROM shared_task_activity_state WHERE shared_task_id={} AND activity_id={} FOR UPDATE",run,activity));
  if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=0||(q.begin()[1]&&ActionNumber(q.begin()[1])!=0))throw std::runtime_error("Fire first action stage differs");
 }
 const int first=kind==RunActionKind::InnerVault?0:kind==RunActionKind::WorshipPair?6:kind==RunActionKind::HelperActivation?13:15;
 const int last=kind==RunActionKind::InnerVault?2:first;
 auto q=ActionQuery(db,fmt::format("SELECT activity_id,done_count,UNIX_TIMESTAMP(completed_time) FROM shared_task_activity_state WHERE shared_task_id={} AND activity_id BETWEEN {} AND {} ORDER BY activity_id FOR UPDATE",run,first,last));
 if(q.RowCount()!=unsigned(last-first+1))throw std::runtime_error("Fire action prior stage missing");
 int i=first;for(auto r=q.begin();r!=q.end();++r,++i)
  if(ActionNumber(r[0])!=unsigned(i)||ActionNumber(r[1])!=1||!r[2]||!ActionNumber(r[2]))throw std::runtime_error("Fire action prior stage differs");
}
}
std::unique_ptr<RunLatchSnapshot> NativeRunActionCustody::PreviewActual(const OriginalDoorUse& use) {
 try {const auto& f=use.Facts();if(!f.mission||!RoleBit(f.kind,f.role))return {};return PreviewLatch(f.kind,*f.mission);}catch(...){return {};}
}
std::unique_ptr<RunLatchSnapshot> NativeRunActionCustody::PreviewActual(const OriginalHelperUse& use) {
 try {const auto& f=use.Facts();if(!RoleBit(RunActionKind::HelperActivation,f.selected_target))return {};return PreviewLatch(RunActionKind::HelperActivation,f.mission);}catch(...){return {};}
}
bool NativeRunActionCustody::RevalidateDoorPlan(Client& c,const PreparedRunDoorPlan& plan) {return plan.RevalidateBeforePersist(c);}
DoorActionResult NativeRunActionCustody::ExecuteOriginalDoor(Client& c,std::unique_ptr<OriginalDoorUse> use,std::unique_ptr<PreparedRunDoorPlan> plan) {
 return FirePersonalOccurrenceSource::NativeItemCustody::DoorSelected(c,std::move(use),std::move(plan));
}
uint64_t NativeRunActionCustody::StageActual(Database& db,uint64_t connection,const RunLatchSnapshot& preview,
 const OriginalDoorUse* door,const OriginalHelperUse* helper,PreparedRunActionOrigins* origins,uint64_t sequence,bool& write_entered) {
 if(bool(door)==bool(helper)||!sequence)throw std::runtime_error("Fire action missing original native issuer");
 const auto kind=helper?RunActionKind::HelperActivation:door->Facts().kind;
 const auto& m=helper?helper->Facts().mission:*door->Facts().mission;
 const auto role=helper?helper->Facts().selected_target:door->Facts().role;
 if(kind!=preview.kind||m.run!=preview.run||!RoleBit(kind,role))throw std::runtime_error("Fire action selection changed");
 ActionSession(db,connection);ActionFoundation(db);LockCurrentRun(db,m);
 auto existing=ActionQuery(db,fmt::format("SELECT id,state,published_role_mask FROM rof2_fire_run_actions WHERE shared_task_id={} AND action_kind={} FOR UPDATE",m.run,unsigned(kind)));
 if(preview.row) {
  if(existing.RowCount()!=1||ActionNumber(existing.begin()[0])!=preview.row||ActionNumber(existing.begin()[1])!=1||!ActionNumber(existing.begin()[2])||
   preview.boundary!=RunActionBoundary::ExistingPublishedRunLatch||kind==RunActionKind::HelperActivation)
   throw std::runtime_error("Fire published latch changed");
  return preview.row;
 }
 if(existing.RowCount())throw std::runtime_error("Fire run action already entered");
 FirstStage(db,m.run,kind);
 PhysicalItemFacts item{};
 uint32_t door_db=0;uint8_t door_packet=0;
 if(helper)item=helper->Facts().clicked;
 else {
  const auto& f=door->Facts();const uint32_t wanted=kind==RunActionKind::InnerVault?47748:kind==RunActionKind::WorshipPair?47762:47757;
  if(f.physical_count>f.physical_items.size())throw std::runtime_error("Fire action physical array differs");
  for(size_t i=0;i<f.physical_count;++i)if(f.physical_items[i].item_id==wanted)item=f.physical_items[i];
  door_db=f.door_database_id;door_packet=f.native_door_id;
 }
 if(!item.item_id||!item.serial||item.units!=1)throw std::runtime_error("Fire first action physical origin missing");
 if(!origins)throw std::runtime_error("Fire action durable origin snapshot missing");
 const auto origin_token=NativeRunItemOrigins::LockActionActual(db,connection,*origins,door,helper);
 write_entered=true; // ANY selected write, before INSERT; unknown/failure is not retry authority.
 auto inserted=ActionQuery(db,fmt::format(
  "INSERT INTO rof2_fire_run_actions (shared_task_id,action_kind,creator_character_id,creator_local_occurrence,creator_accepted_time,"
  "native_sequence,dynamic_zone_id,zone_id,instance_id,source_role,source_door_sql_id,source_door_packet_id,source_item_id,"
  "source_origin_token,source_item_serial,source_item_slot,source_item_charges,inventory_debit_committed,state,published_role_mask,helper_entity_id,"
  "committed_at,native_published_at,terminal_at) VALUES ({},{},{},{},{},{},{},{},{},{},{},{},{},UNHEX('{}'),{},{},{},{},0,0,NULL,UNIX_TIMESTAMP(),NULL,NULL)",
  m.run,unsigned(kind),m.character,m.local_occurrence,m.accepted_time,sequence,m.dynamic_zone,m.zone,m.instance,unsigned(role),
  door_db?std::to_string(door_db):"NULL",door_packet?std::to_string(door_packet):"NULL",item.item_id,origin_token,item.serial,item.slot,item.charges,preview.debit?1:0));
 if(inserted.RowsAffected()!=1||!inserted.LastInsertedID())throw std::runtime_error("Fire action insert not exact");
 ActionSession(db,connection);return inserted.LastInsertedID();
}
bool NativeRunActionCustody::AcknowledgeNativePublication(NativeRunPublication& proof) {
 if(proof.taken_)return false;
 proof.taken_=true; // FIRST: the private positive scene event is one-use even on failure.
 const auto p=proof.facts_;
 const auto bit=RoleBit(p.kind,p.role);
 if(!proof.state_||!bit||!p.run_action_id||!p.run_id||!p.native_sequence||!p.character||p.zone!=490||!p.instance||!p.door_database_id||!p.native_door_id)return false;
 bool transaction=false;
 try {
  auto fresh=ActionQuery(database,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(fresh.RowCount()!=1||ActionNumber(fresh.begin()[1])||ActionNumber(fresh.begin()[2])!=1)return false;
  const auto connection=ActionNumber(fresh.begin()[0]);
  if(!database.TransactionBeginStrict().Success())return false;
  transaction=true;ActionFoundation(database);
  auto q=ActionQuery(database,fmt::format("SELECT creator_character_id,creator_local_occurrence,creator_accepted_time,native_sequence,dynamic_zone_id,"
   "zone_id,instance_id,source_role,source_door_sql_id,source_door_packet_id,state,published_role_mask FROM rof2_fire_run_actions "
   "WHERE id={} AND shared_task_id={} AND action_kind={} FOR UPDATE",p.run_action_id,p.run_id,unsigned(p.kind)));
  if(q.RowCount()!=1)throw std::runtime_error("Fire publication exact action missing");
  auto r=q.begin();const auto state=ActionNumber(r[10]);
  if(ActionNumber(r[5])!=p.zone||ActionNumber(r[6])!=p.instance||state>1)throw std::runtime_error("Fire publication row stage differs");
  const bool helper=p.kind==RunActionKind::HelperActivation;
  if((state==0||helper)&&(ActionNumber(r[0])!=p.character||ActionNumber(r[3])!=p.native_sequence))throw std::runtime_error("Fire original publication issuer differs");
  if(helper&&(!p.actor_entity||state!=0||ActionNumber(r[7])!=unsigned(p.role)))throw std::runtime_error("Fire original helper publication differs");
  if(!helper&&(state==0||p.kind!=RunActionKind::WorshipPair)&&
   (ActionNumber(r[7])!=unsigned(p.role)||!r[8]||!r[9]||ActionNumber(r[8])!=p.door_database_id||ActionNumber(r[9])!=p.native_door_id))
   throw std::runtime_error("Fire original door publication differs");
  // Original first issuer must retain its same accepted epoch. A later genuine
  // published-latch opener can be another current member; never copies key custody.
  auto member=ActionQuery(database,fmt::format("SELECT h.occurrence_id,UNIX_TIMESTAMP(s.accepted_time) FROM shared_tasks s "
   "JOIN shared_task_members m ON m.shared_task_id=s.id AND m.character_id={} "
   "JOIN character_shared_task_reward_instances h ON h.shared_task_id=s.id AND h.character_id=m.character_id AND h.task_id=s.task_id "
   "JOIN character_task_reward_instances a ON a.character_id=h.character_id AND a.task_id=h.task_id AND a.occurrence_id=h.occurrence_id "
   "WHERE s.id={} AND s.task_id=920170 AND s.completion_time IS NULL AND a.accepted_time=UNIX_TIMESTAMP(s.accepted_time) FOR UPDATE",p.character,p.run_id));
  if(member.RowCount()!=1||!ActionNumber(member.begin()[0])||
   (state==0&&(ActionNumber(member.begin()[0])!=ActionNumber(r[1])||ActionNumber(member.begin()[1])!=ActionNumber(r[2]))))
   throw std::runtime_error("Fire native publication current run member differs");
  const auto mask=ActionNumber(r[11])|bit;
  auto updated=ActionQuery(database,fmt::format("UPDATE rof2_fire_run_actions SET state={},published_role_mask={},helper_entity_id={},"
   "native_published_at=COALESCE(native_published_at,UNIX_TIMESTAMP()),terminal_at={} WHERE id={} AND shared_task_id={} AND action_kind={} AND state={}",
   helper?2:1,mask,helper?std::to_string(p.actor_entity):"NULL",helper?"UNIX_TIMESTAMP()":"NULL",p.run_action_id,p.run_id,unsigned(p.kind),state));
  // A second real opening of an already-published same leaf is idempotent;
  // no second debit, actor, progress or inferred publication event is created.
  if(updated.RowsAffected()>1)throw std::runtime_error("Fire publication ack row mismatch");
  ActionSession(database,connection);const auto commit=database.TransactionCommitStrict();transaction=false;
  return commit.Success();
 }catch(...){if(transaction)database.TransactionRollbackStrict();return false;}
}
} // namespace FireRunActionSource
