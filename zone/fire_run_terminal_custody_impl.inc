// Included only by the existing feature-private native item custody writer.
namespace FireRunActionSource {
struct PublishedTerminalReturnReceipt::State {
 const Client* owner=nullptr;bool taken=false;
 uint64_t row=0,run=0,sequence=0;
 std::unique_ptr<OriginalMoldTrade> original;
 std::unique_ptr<PreparedTerminalChestPlan> plan;
};
PublishedTerminalReturnReceipt::PublishedTerminalReturnReceipt(std::unique_ptr<State> s)noexcept:state_(std::move(s)){}
PublishedTerminalReturnReceipt::~PublishedTerminalReturnReceipt()=default;
PublishedTerminalReturnReceipt::PublishedTerminalReturnReceipt(PublishedTerminalReturnReceipt&& p)noexcept:state_(std::move(p.state_)){}
std::optional<PublishedTerminalReturnView> PublishedTerminalReturnReceipt::Take(const Client& c)noexcept {
 if(!state_||state_->taken||state_->owner!=&c)return {};state_->taken=true;
 return PublishedTerminalReturnView{state_->original.get(),state_->plan.get(),state_->row,state_->run,state_->sequence};
}
namespace {
constexpr unsigned TerminalKind=6; // finite existing run/action ledger, no reward path
struct GuardianCustody {uint64_t character,occurrence,accepted,sequence,npc,spawn,actor,corpse;};
std::vector<std::string> TerminalParts(const std::string& text,char separator,size_t maximum) {
 if(text.size()>2048)throw std::runtime_error("Fire terminal snapshot too large");
 std::vector<std::string> parts;size_t begin=0;
 for(;;){const auto end=text.find(separator,begin);parts.push_back(text.substr(begin,end==std::string::npos?end:end-begin));
  if(parts.size()>maximum)throw std::runtime_error("Fire terminal snapshot count differs");
  if(end==std::string::npos)return parts;begin=end+1;}
}
GuardianCustody GuardianSnapshot(const char* text) {
 if(!text)throw std::runtime_error("Fire actual Guardian custody absent");
 const auto p=TerminalParts(text,'|',9);
 if(p.size()!=9||p[0]!="g1")throw std::runtime_error("Fire Guardian snapshot shape differs");
 GuardianCustody g{ActionNumber(p[1].c_str()),ActionNumber(p[2].c_str()),ActionNumber(p[3].c_str()),ActionNumber(p[4].c_str()),
  ActionNumber(p[5].c_str()),ActionNumber(p[6].c_str()),ActionNumber(p[7].c_str()),ActionNumber(p[8].c_str())};
 if(!g.character||!g.occurrence||!g.accepted||!g.sequence||g.npc!=490911||g.spawn!=304062014||!g.actor||!g.corpse)
  throw std::runtime_error("Fire Guardian original facts differ");
 return g;
}
std::string MemberSnapshot(const TerminalReturnFacts& f) {
 if(!f.instance_expire_at||f.accepted_members.empty()||f.accepted_members.size()>6)
  throw std::runtime_error("Fire terminal accepted snapshot missing");
 std::string text="m1|"+std::to_string(f.instance_expire_at);uint32_t last=0;bool giver=false;
 for(const auto& m:f.accepted_members) {
  if(m.character<=last||!m.local_occurrence||m.accepted_time!=f.mission.accepted_time)
   throw std::runtime_error("Fire terminal accepted snapshot differs");
  text+="|"+std::to_string(m.character)+":"+std::to_string(m.local_occurrence)+":"+std::to_string(m.accepted_time);last=m.character;
  if(m.character==f.mission.character&&m.local_occurrence==f.mission.local_occurrence)giver=true;
 }
 if(!giver)throw std::runtime_error("Fire returning holder outside accepted snapshot");return text;
}
void RequireMemberSnapshot(const char* raw,const MissionRunFacts& current,uint64_t now) {
 if(!raw)throw std::runtime_error("Fire terminal accepted snapshot absent");
 auto p=TerminalParts(raw,'|',8);
 if(p.size()<3||p[0]!="m1"||ActionNumber(p[1].c_str())<=now)
  throw std::runtime_error("Fire terminal original instance lease expired");
 bool found=false;uint64_t previous=0;
 for(size_t i=2;i<p.size();++i) {
  auto m=TerminalParts(p[i],':',3);if(m.size()!=3)throw std::runtime_error("Fire terminal member shape differs");
  const auto id=ActionNumber(m[0].c_str()),occ=ActionNumber(m[1].c_str()),accepted=ActionNumber(m[2].c_str());
  if(id<=previous||!occ||accepted!=current.accepted_time)throw std::runtime_error("Fire terminal member epoch differs");
  previous=id;if(id==current.character&&occ==current.local_occurrence)found=true;
 }
 if(!found)throw std::runtime_error("Fire opener outside original accepted snapshot");
}
void RequireTerminalMembers(Database& db,const TerminalReturnFacts& f) {
 auto q=ActionQuery(db,fmt::format("SELECT m.character_id,h.occurrence_id,r.accepted_time FROM shared_task_members m "
  "JOIN character_shared_task_reward_instances h ON h.shared_task_id=m.shared_task_id AND h.character_id=m.character_id AND h.task_id=920170 "
  "JOIN character_task_reward_instances r ON r.character_id=h.character_id AND r.task_id=h.task_id AND r.occurrence_id=h.occurrence_id "
  "WHERE m.shared_task_id={} ORDER BY m.character_id FOR UPDATE",f.mission.run));
 if(q.RowCount()!=f.accepted_members.size())throw std::runtime_error("Fire terminal member universe changed");
 size_t i=0;for(auto row=q.begin();row!=q.end();++row,++i) {
  const auto& m=f.accepted_members[i];if(ActionNumber(row[0])!=m.character||ActionNumber(row[1])!=m.local_occurrence||
   ActionNumber(row[2])!=m.accepted_time)throw std::runtime_error("Fire terminal original member changed");
 }
 q=ActionQuery(db,fmt::format("SELECT expire_at FROM instance_list WHERE id={} AND zone=490 AND version=1 AND never_expires=0 "
  "AND expire_at={} AND expire_at>UNIX_TIMESTAMP() FOR UPDATE",f.mission.instance,f.instance_expire_at));
 if(q.RowCount()!=1)throw std::runtime_error("Fire terminal original instance lease changed");
}
void RequireTerminalStage(Database& db,uint64_t run,int goal) {
 auto q=ActionQuery(db,fmt::format("SELECT activity_id,done_count,UNIX_TIMESTAMP(completed_time) FROM shared_task_activity_state "
  "WHERE shared_task_id={} AND activity_id BETWEEN {} AND {} ORDER BY activity_id FOR UPDATE",run,16,goal));
 if(q.RowCount()!=unsigned(goal==18?3:1))throw std::runtime_error("Fire terminal activity shape differs");
 int index=16;for(auto row=q.begin();row!=q.end();++row,++index) {
  const bool completed=goal==18&&index<18;
  if(ActionNumber(row[0])!=unsigned(index)||ActionNumber(row[1])!=(completed?1:0)||
   (completed?(!row[2]||!ActionNumber(row[2])):(row[2]&&ActionNumber(row[2]))))
   throw std::runtime_error("Fire terminal actual activity state differs");
 }
}
void RequireActualGuardianLoot(Database& db,const RunLootFacts& p) {
 ActionFoundation(db);
 auto q=ActionQuery(db,fmt::format("SELECT guardian_death_snapshot,state,inventory_debit_committed FROM rof2_fire_run_actions "
  "WHERE shared_task_id={} AND action_kind=6 FOR UPDATE",p.mission.run));
 if(q.RowCount()!=1||ActionNumber(q.begin()[1])||ActionNumber(q.begin()[2]))
  throw std::runtime_error("Fire mold lacks actual unreturned Guardian custody");
 const auto g=GuardianSnapshot(q.begin()[0]);
 if(p.source_npc_type!=g.npc||p.source_spawn!=g.spawn||p.source_death_sequence!=g.sequence||p.source_corpse_entity!=g.corpse)
  throw std::runtime_error("Fire mold original Guardian/corpse sequence differs");
}
uint64_t StageMoldReturnActual(Database& db,uint64_t connection,const OriginalMoldTrade& original,
 const std::string& token,bool& write_entered) {
 const auto& f=original.Facts();ActionSession(db,connection);ActionFoundation(db);OriginFoundation(db);LockCurrentRun(db,f.mission);
 RequireTerminalStage(db,f.mission.run,18);RequireTerminalMembers(db,f);
 auto q=ActionQuery(db,fmt::format("SELECT id,guardian_death_snapshot,state,inventory_debit_committed FROM rof2_fire_run_actions "
  "WHERE shared_task_id={} AND action_kind=6 FOR UPDATE",f.mission.run));
 if(q.RowCount()!=1||ActionNumber(q.begin()[2])||ActionNumber(q.begin()[3]))
  throw std::runtime_error("Fire terminal already reserved or lacks genuine Guardian");
 const auto row=ActionNumber(q.begin()[0]);const auto g=GuardianSnapshot(q.begin()[1]);
 RequirePublishedOrigin(db,f.mission,47758,token,true);
 q=ActionQuery(db,fmt::format("SELECT source_npc_type_id,source_corpse_entity_id,source_death_sequence FROM rof2_fire_run_item_origins "
  "WHERE token=UNHEX('{}') AND shared_task_id={} AND item_id=47758 AND origin_kind=1 AND state=1 FOR UPDATE",token,f.mission.run));
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=g.npc||ActionNumber(q.begin()[1])!=g.corpse||ActionNumber(q.begin()[2])!=g.sequence)
  throw std::runtime_error("Fire returning mold lacks same original Guardian origin");
 const auto members=MemberSnapshot(f);write_entered=true;
 q=ActionQuery(db,fmt::format("UPDATE rof2_fire_run_actions SET creator_character_id={},creator_local_occurrence={},creator_accepted_time={},"
  "native_sequence={},source_item_id=47758,source_origin_token=UNHEX('{}'),source_item_serial={},source_item_slot={},source_item_charges={},"
  "inventory_debit_committed=1,accepted_member_snapshot='{}',committed_at=UNIX_TIMESTAMP() "
  "WHERE id={} AND shared_task_id={} AND action_kind=6 AND state=0 AND inventory_debit_committed=0",
  f.mission.character,f.mission.local_occurrence,f.mission.accepted_time,f.native_sequence,token,f.mold.serial,f.mold.slot,f.mold.charges,
  members,row,f.mission.run)); // snapshot is finite canonical digits/separators only
 if(q.RowsAffected()!=1)throw std::runtime_error("Fire terminal reservation changed");ActionSession(db,connection);return row;
}
void ConsumeMoldOriginActual(Database& db,uint64_t connection,const TerminalReturnFacts& f,const std::string& token,uint64_t row) {
 ActionSession(db,connection);
 auto q=ActionQuery(db,fmt::format("UPDATE rof2_fire_run_item_origins SET state=2,consumed_at=UNIX_TIMESTAMP(),consumed_action_id={},consumed_by_character_id={} "
  "WHERE token=UNHEX('{}') AND shared_task_id={} AND item_id=47758 AND state=1",row,f.mission.character,token,f.mission.run));
 if(q.RowsAffected()!=1)throw std::runtime_error("Fire original mold consumption changed");ActionSession(db,connection);
}
}
bool NativeRunTerminalCustody::ValidateOriginalMoldReturn(Client& c,const OriginalMoldTrade& original) {
 try {
  const auto& f=original.Facts();const auto& p=f.mold;
  if(!ValidMission(f.mission)||f.mission.character!=c.CharacterID()||f.client_entity!=c.GetID()||!f.native_sequence||
   f.giver_npc!=490912||f.giver_spawn!=304062015||!f.giver_entity||p.item_id!=47758||!p.serial||p.units!=1||
   !EQ::ValueWithin(p.slot,EQ::invslot::TRADE_BEGIN,EQ::invslot::TRADE_NPC_END)||!zone||
   zone->GetZoneID()!=f.mission.zone||zone->GetInstanceID()!=f.mission.instance)return false;
  const auto* item=c.GetInv().GetItem(p.slot);
  if(!item||!OriginShape(*item)||item->GetID()!=47758||item->GetSerialNumber()!=p.serial||item->GetCharges()!=p.charges)return false;
  for(int16_t slot=EQ::invslot::TRADE_BEGIN;slot<=EQ::invslot::TRADE_NPC_END;++slot)
   if(slot!=p.slot&&c.GetInv().GetItem(slot))return false;
  RequirePublishedOrigin(database,f.mission,47758,ItemOriginToken(*item),false);return true;
 }catch(...){return false;}
}
OriginalTerminalReturnResult NativeRunTerminalCustody::ExecuteOriginalReturn(Client& c,
 std::unique_ptr<OriginalMoldTrade> original,std::unique_ptr<PreparedTerminalChestPlan> plan) {
 return FirePersonalOccurrenceSource::NativeItemCustody::MoldReturnSelected(c,std::move(original),std::move(plan));
}
bool NativeRunTerminalCustody::AcknowledgeOriginalDeath(OriginalTerminalDeathProof& proof) {
 if(proof.taken_)return false;proof.taken_=true; // FIRST, including every error path
 const auto p=proof.Facts();const auto& m=p.mission;
 if(!proof.state_||!ValidMission(m)||!p.native_sequence||p.terminal_row||p.original_chest_open||
  p.npc_type!=490911||p.original_spawn!=304062014||!p.original_entity||!p.corpse_entity)return false;
 bool transaction=false;
 try {
  const auto snapshot=fmt::format("g1|{}|{}|{}|{}|{}|{}|{}|{}",m.character,m.local_occurrence,m.accepted_time,
   p.native_sequence,p.npc_type,p.original_spawn,p.original_entity,p.corpse_entity);
  auto q=ActionQuery(database,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(q.RowCount()!=1||ActionNumber(q.begin()[1])||ActionNumber(q.begin()[2])!=1)return false;
  const auto connection=ActionNumber(q.begin()[0]);if(!database.TransactionBeginStrict().Success())return false;
  transaction=true;ActionFoundation(database);LockCurrentRun(database,m);RequireTerminalStage(database,m.run,16);
  q=ActionQuery(database,fmt::format("SELECT id FROM rof2_fire_run_actions WHERE shared_task_id={} AND action_kind=6 FOR UPDATE",m.run));
  if(q.RowCount())throw std::runtime_error("Fire genuine Guardian already acknowledged; no replay");
  q=ActionQuery(database,fmt::format("INSERT INTO rof2_fire_run_actions "
   "(shared_task_id,action_kind,creator_character_id,creator_local_occurrence,creator_accepted_time,native_sequence,dynamic_zone_id,zone_id,instance_id,"
   "source_role,source_item_id,source_origin_token,source_item_serial,source_item_slot,source_item_charges,inventory_debit_committed,state,guardian_death_snapshot,committed_at) "
   "VALUES ({},6,{},{},{},{},{},{},{},0,NULL,NULL,NULL,NULL,NULL,0,0,'{}',UNIX_TIMESTAMP())",
   m.run,m.character,m.local_occurrence,m.accepted_time,p.native_sequence,m.dynamic_zone,m.zone,m.instance,snapshot));
  if(q.RowsAffected()!=1||!q.LastInsertedID())throw std::runtime_error("Fire Guardian custody insert differs");
  ActionSession(database,connection);const auto commit=database.TransactionCommitStrict();transaction=false;return commit.Success();
 }catch(...){if(transaction)database.TransactionRollbackStrict();return false;}
}
bool NativeRunTerminalCustody::AcknowledgeNativePublication(NativeTerminalPublication& proof) {
 if(proof.taken_)return false;proof.taken_=true;
 const auto p=proof.Facts();const auto& m=p.mission;
 if(!proof.owner_||!ValidMission(m)||!p.terminal_row||!p.original_accept_sequence||!p.actor_entity)return false;
 const bool registration=p.boundary==TerminalNativeBoundary::Registered;
 if(!registration&&p.boundary!=TerminalNativeBoundary::OpenedDeath)return false;
 if(registration?(p.corpse_entity||p.native_death_sequence):(!p.corpse_entity||!p.native_death_sequence))return false;
 bool transaction=false;
 try {
  auto q=ActionQuery(database,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit,UNIX_TIMESTAMP()");
  if(q.RowCount()!=1||ActionNumber(q.begin()[1])||ActionNumber(q.begin()[2])!=1)return false;
  const auto connection=ActionNumber(q.begin()[0]),now=ActionNumber(q.begin()[3]);
  if(!database.TransactionBeginStrict().Success())return false;transaction=true;ActionFoundation(database);LockCurrentRun(database,m);
  q=ActionQuery(database,fmt::format("SELECT creator_character_id,creator_local_occurrence,creator_accepted_time,native_sequence,dynamic_zone_id,zone_id,instance_id,"
   "inventory_debit_committed,state,terminal_actor_entity_id,terminal_corpse_entity_id,accepted_member_snapshot FROM rof2_fire_run_actions "
   "WHERE id={} AND shared_task_id={} AND action_kind=6 FOR UPDATE",p.terminal_row,m.run));
  if(q.RowCount()!=1)throw std::runtime_error("Fire terminal exact custody row missing");auto row=q.begin();
  if(ActionNumber(row[3])!=p.original_accept_sequence||ActionNumber(row[4])!=m.dynamic_zone||ActionNumber(row[5])!=m.zone||
   ActionNumber(row[6])!=m.instance||ActionNumber(row[7])!=1||ActionNumber(row[8])!=(registration?0:1))
   throw std::runtime_error("Fire terminal publication custody differs");
  RequireMemberSnapshot(row[11],m,now);
  if(registration) {
   if(ActionNumber(row[0])!=m.character||ActionNumber(row[1])!=m.local_occurrence||ActionNumber(row[2])!=m.accepted_time||row[9]||row[10])
    throw std::runtime_error("Fire terminal original registration differs");
  }else if(!row[9]||ActionNumber(row[9])!=p.actor_entity||row[10])throw std::runtime_error("Fire terminal original opened actor differs");
  q=ActionQuery(database,fmt::format("UPDATE rof2_fire_run_actions SET state={},terminal_actor_entity_id={},terminal_corpse_entity_id={},"
   "native_published_at=COALESCE(native_published_at,UNIX_TIMESTAMP()),terminal_at={} WHERE id={} AND shared_task_id={} AND action_kind=6 "
   "AND state={} AND inventory_debit_committed=1",registration?1:2,p.actor_entity,registration?"NULL":std::to_string(p.corpse_entity),
   registration?"NULL":"UNIX_TIMESTAMP()",p.terminal_row,m.run,registration?0:1));
  if(q.RowsAffected()!=1)throw std::runtime_error("Fire terminal positive publication changed");
  ActionSession(database,connection);const auto commit=database.TransactionCommitStrict();transaction=false;return commit.Success();
 }catch(...){if(transaction)database.TransactionRollbackStrict();return false;}
}
} // namespace FireRunActionSource
