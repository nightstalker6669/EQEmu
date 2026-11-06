// Included only in the feature-private native writer, after action ledger helpers.
// Durable current-run provenance is independent of process/runtime item serials.
namespace FireRunActionSource {
namespace {void RequireActualGuardianLoot(Database&,const RunLootFacts&);}
namespace {
bool OriginItem(uint32_t id) noexcept {
 switch(id) {case 47748:case 47749:case 47750:case 47751:case 47752:
 case 47753:case 47754:case 47755:case 47756:case 47757:case 47760:
 case 47761:case 47762:case 47758:return true;default:return false;}
}
bool OriginShape(const EQ::ItemInstance& item) {
 return item.GetItem()&&OriginItem(item.GetID())&&item.IsClassCommon()&&
  !item.IsStackable()&&item.GetContents()->empty()&&!item.IsEvolving()&&
  !item.GetEvolveUniqueID()&&!item.GetHousingHouseID()&&!item.GetHousingCrateID();
}
bool OriginToken(const std::string& token) noexcept {
 return token.size()==32&&token.find_first_not_of("0123456789abcdef")==std::string::npos;
}
std::string ItemOriginToken(const EQ::ItemInstance& item) {
 // Native getter only finds an existing key; it does not mutate the map.
 return const_cast<EQ::ItemInstance&>(item).GetCustomData(RunOriginSealKey);
}
std::string NewOriginToken() {
 auto token=EQ::Util::UUID::Generate().ToString();
 token.erase(std::remove(token.begin(),token.end(),'-'),token.end());
 if(!OriginToken(token))throw std::runtime_error("Fire native UUID shape differs");
 return token;
}
bool OriginHeldActual(Client& c,const EQ::ItemInstance& item,int16_t slot,const MissionRunFacts& m) {
 if(!ValidMission(m)||m.character!=c.CharacterID()||!zone||zone->GetZoneID()!=m.zone||
  zone->GetInstanceID()!=m.instance||!OriginShape(item)||c.GetInv().GetItem(slot)!=&item)return false;
 if(slot==EQ::invslot::slotCursor)return true; // visible original cursor only
 const auto root=EQ::ValueWithin(slot,EQ::invbag::GENERAL_BAGS_BEGIN,EQ::invbag::GENERAL_BAGS_END)?
  EQ::InventoryProfile::CalcSlotId(slot):slot;
 return EQ::ValueWithin(root,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END)&&
  c.GetInv().GetLookup()&&((uint64_t(1)<<root)&c.GetInv().GetLookup()->PossessionsBitmask);
}
void OriginFoundation(Database& db) {
 auto q=ActionQuery(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='rof2_fire_run_item_origins' AND engine='InnoDB'");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=1)throw std::runtime_error("Fire origin engine differs");
 q=ActionQuery(db,"SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='rof2_fire_run_item_origins'");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=21)throw std::runtime_error("Fire origin schema differs");
 q=ActionQuery(db,"SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='rof2_fire_run_item_origins' AND index_name='PRIMARY' AND non_unique=0 AND seq_in_index=1 AND column_name='token'");
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=1)throw std::runtime_error("Fire origin primary key differs");
}
void RequirePublishedOrigin(Database& db,const MissionRunFacts& m,uint32_t item,const std::string& token,bool lock) {
 if(!ValidMission(m)||!OriginItem(item)||!OriginToken(token))throw std::runtime_error("Fire original seal absent");
 auto q=ActionQuery(db,fmt::format("SELECT item_id,state FROM rof2_fire_run_item_origins WHERE token=UNHEX('{}') AND shared_task_id={}{}",token,m.run,lock?" FOR UPDATE":""));
 if(q.RowCount()!=1||ActionNumber(q.begin()[0])!=item||ActionNumber(q.begin()[1])!=1)
  throw std::runtime_error("Fire origin not positively published in current run");
}
}
struct PreparedRunTransformOrigins::State {
 struct Input {int16_t slot;uint32_t item;const EQ::ItemInstance* actual;std::string token;};
 MissionRunFacts mission;
 uint64_t sequence=0;
 uint32_t output_item=0;
 int32_t output_serial=0;
 int16_t output_slot=-1;
 uint8_t kind=0;
 const Client* owner=nullptr;
 std::string output_token;
 std::vector<Input> inputs;
 bool staged=false,publication_taken=false;
};
PreparedRunTransformOrigins::PreparedRunTransformOrigins(std::unique_ptr<State> s)noexcept:state_(std::move(s)){}
PreparedRunTransformOrigins::~PreparedRunTransformOrigins()=default;
bool NativeRunItemOrigins::HasActualPublishedOrigin(Client& c,const EQ::ItemInstance& item,int16_t slot,const MissionRunFacts& m) {
 try {
  if(!OriginHeldActual(c,item,slot,m))return false;
  RequirePublishedOrigin(database,m,item.GetID(),ItemOriginToken(item),false);return true;
 }catch(...){return false;}
}
std::unique_ptr<PreparedRunTransformOrigins> NativeRunItemOrigins::PrepareTransformActual(Client& c,
 const FireSharedTransformSource::OriginalSharedTransformUse& original,
 const std::vector<int16_t>& slots,EQ::ItemInstance& output,uint64_t sequence,int16_t output_slot) {
 try {
  const auto& shared=original.shared;
  const auto rule=FireSharedTransformSource::OriginalRule(original.item_id);
  if(!rule||rule->clicked_item!=shared.rule.clicked_item||rule->output_item!=shared.rule.output_item||
   rule->input_count!=shared.rule.input_count||rule->inputs!=shared.rule.inputs||
   slots.size()!=rule->input_count||!OriginShape(output)||output.GetID()!=rule->output_item||
   !ItemOriginToken(output).empty())return {};
  auto s=std::make_unique<PreparedRunTransformOrigins::State>();
  s->mission={shared.run,shared.local_occurrence,shared.character,shared.accepted_time,
   shared.zone_id,1,shared.instance_id,shared.dynamic_zone_id};
  if(!ValidMission(s->mission)||!sequence||original.slot!=slots.front())return {};
  s->sequence=sequence;s->output_slot=output_slot;
  s->owner=&c;s->kind=output.GetID()==47762?2:3;
  s->output_item=output.GetID();s->output_serial=output.GetSerialNumber();
  s->inputs.reserve(slots.size());
  std::array<bool,5> seen{};
  for(const auto slot:slots) {
   const auto* item=c.GetInv().GetItem(slot);
   if(!item||!OriginHeldActual(c,*item,slot,s->mission))return {};
   size_t i=0;for(;i<rule->input_count;++i)if(rule->inputs[i]==item->GetID())break;
   if(i==rule->input_count||seen[i])return {};seen[i]=true;
   auto token=ItemOriginToken(*item);
   RequirePublishedOrigin(database,s->mission,item->GetID(),token,false);
   for(const auto& prior:s->inputs)if(prior.slot==slot||prior.token==token)return {};
   s->inputs.push_back({slot,item->GetID(),item,std::move(token)});
  }
  s->output_token=NewOriginToken();output.SetCustomData(RunOriginSealKey,s->output_token);
  if(ItemOriginToken(output)!=s->output_token)return {};
  return std::unique_ptr<PreparedRunTransformOrigins>(new PreparedRunTransformOrigins(std::move(s)));
 }catch(...){return {};}
}
bool NativeRunItemOrigins::RevalidateTransformActual(Client& c,const PreparedRunTransformOrigins& prepared) {
 if(!prepared.state_||prepared.state_->owner!=&c||prepared.state_->staged)return false;
 const auto& s=*prepared.state_;
 for(const auto& input:s.inputs)if(!input.actual||c.GetInv().GetItem(input.slot)!=input.actual||!OriginHeldActual(c,*input.actual,input.slot,s.mission)||
  input.actual->GetID()!=input.item||ItemOriginToken(*input.actual)!=input.token)return false;
 return true;
}
void NativeRunItemOrigins::StageTransformActual(Database& db,uint64_t connection,
 PreparedRunTransformOrigins& prepared,bool& write_entered) {
 if(!prepared.state_||prepared.state_->staged)throw std::runtime_error("Fire transform origins already entered");
 auto& s=*prepared.state_;
 if(!s.sequence)throw std::runtime_error("Fire actual transform sequence absent");
 ActionSession(db,connection);OriginFoundation(db);LockCurrentRun(db,s.mission);
 // All inputs are locked and proved before the first selected write. Producer
 // character is audit only: an actual same-run member may hold transferred loot.
 for(const auto& input:s.inputs)RequirePublishedOrigin(db,s.mission,input.item,input.token,true);
 write_entered=true;s.staged=true;
 auto q=ActionQuery(db,fmt::format("INSERT INTO rof2_fire_run_item_origins "
  "(token,shared_task_id,item_id,origin_kind,state,producer_character_id,producer_local_occurrence,producer_accepted_time,producer_native_sequence,source_death_sequence,"
  "source_npc_type_id,source_corpse_entity_id,source_loot_slot,native_item_serial,native_item_slot,created_at,published_at,consumed_at,consumed_action_id,consumed_output_token,consumed_by_character_id) "
  "VALUES (UNHEX('{}'),{},{},{},0,{},{},{},{},NULL,NULL,NULL,NULL,{},{},UNIX_TIMESTAMP(),NULL,NULL,NULL,NULL,NULL)",
  s.output_token,s.mission.run,s.output_item,unsigned(s.kind),s.mission.character,s.mission.local_occurrence,s.mission.accepted_time,s.sequence,s.output_serial,s.output_slot));
 if(q.RowsAffected()!=1)throw std::runtime_error("Fire transform output origin insert differs");
 for(const auto& input:s.inputs) {
  q=ActionQuery(db,fmt::format("UPDATE rof2_fire_run_item_origins SET state=2,consumed_at=UNIX_TIMESTAMP(),consumed_output_token=UNHEX('{}'),consumed_by_character_id={} "
   "WHERE token=UNHEX('{}') AND shared_task_id={} AND item_id={} AND state=1",s.output_token,s.mission.character,input.token,s.mission.run,input.item));
  if(q.RowsAffected()!=1)throw std::runtime_error("Fire transform input origin consumption differs");
 }
 ActionSession(db,connection);
}
bool NativeRunItemOrigins::PublishTransformActual(Client& c,PreparedRunTransformOrigins& prepared,
 const EQ::ItemInstance& output,int16_t slot) {
 if(!prepared.state_||prepared.state_->publication_taken)return false;
 auto& s=*prepared.state_;s.publication_taken=true; // seal FIRST; no retry on failure
 bool held=c.GetInv().GetItem(slot)==&output;
 if(slot==EQ::invslot::slotCursor) {
  held=false;for(auto it=c.GetInv().cursor_cbegin();it!=c.GetInv().cursor_cend();++it)if(*it==&output){held=true;break;}
 }
 if(!s.staged||s.owner!=&c||!held||slot!=s.output_slot||!OriginShape(output)||output.GetID()!=s.output_item||
  output.GetSerialNumber()!=s.output_serial||ItemOriginToken(output)!=s.output_token||
  !ValidMission(s.mission)||c.CharacterID()!=s.mission.character||!zone||
  zone->GetZoneID()!=s.mission.zone||zone->GetInstanceID()!=s.mission.instance)return false;
 bool transaction=false;
 try {
  auto q=ActionQuery(database,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(q.RowCount()!=1||ActionNumber(q.begin()[1])||ActionNumber(q.begin()[2])!=1)return false;
  const auto connection=ActionNumber(q.begin()[0]);
  if(!database.TransactionBeginStrict().Success())return false;
  transaction=true;OriginFoundation(database);LockCurrentRun(database,s.mission);
  q=ActionQuery(database,fmt::format("UPDATE rof2_fire_run_item_origins SET state=1,published_at=UNIX_TIMESTAMP(),native_item_slot={} "
   "WHERE token=UNHEX('{}') AND shared_task_id={} AND item_id={} AND origin_kind={} AND state=0 AND producer_character_id={} "
   "AND producer_local_occurrence={} AND producer_accepted_time={} AND producer_native_sequence={}",slot,s.output_token,s.mission.run,s.output_item,unsigned(s.kind),
   s.mission.character,s.mission.local_occurrence,s.mission.accepted_time,s.sequence));
  if(q.RowsAffected()!=1)throw std::runtime_error("Fire published transform origin differs");
  ActionSession(database,connection);const auto commit=database.TransactionCommitStrict();transaction=false;return commit.Success();
 }catch(...){if(transaction)database.TransactionRollbackStrict();return false;}
}
struct PreparedRunActionOrigins::State {
 struct Input {PhysicalItemFacts facts;const EQ::ItemInstance* actual;std::string token;};
 MissionRunFacts mission;
 RunActionKind kind{};
 const OriginalDoorUse* door=nullptr;
 const OriginalHelperUse* helper=nullptr;
 const Client* owner=nullptr;
 std::vector<Input> inputs;
 bool locked=false,consumed=false;
};
PreparedRunActionOrigins::PreparedRunActionOrigins(std::unique_ptr<State> s)noexcept:state_(std::move(s)){}
PreparedRunActionOrigins::~PreparedRunActionOrigins()=default;
std::unique_ptr<PreparedRunActionOrigins> NativeRunItemOrigins::PrepareActionActual(Client& c,
 const OriginalDoorUse* door,const OriginalHelperUse* helper) {
 try {
  if(bool(door)==bool(helper)||(door&&!door->Facts().mission))return {};
  auto s=std::make_unique<PreparedRunActionOrigins::State>();
  s->owner=&c;s->door=door;s->helper=helper;
  s->kind=helper?RunActionKind::HelperActivation:door->Facts().kind;
  s->mission=helper?helper->Facts().mission:*door->Facts().mission;
  std::array<uint32_t,3> wanted{};
  size_t count=0;
  switch(s->kind) {
   case RunActionKind::InnerVault:wanted={{47748,47749,47750}};count=3;break;
   case RunActionKind::WorshipPair:wanted[0]=47762;count=1;break;
   case RunActionKind::GuardianVaultKey:wanted[0]=47757;count=1;break;
   case RunActionKind::HelperActivation:wanted[0]=47756;count=1;break;
   default:return {};
  }
  if(door&&(door->Facts().physical_count!=count||door->Facts().required_count!=count))return {};
  s->inputs.reserve(count);
  // Canonical finite item order also bounds lock order and source_token meaning.
  for(size_t i=0;i<count;++i) {
   const PhysicalItemFacts* fact=nullptr;
   if(helper)fact=&helper->Facts().clicked;
   else for(size_t j=0;j<count;++j)if(door->Facts().physical_items[j].item_id==wanted[i]) {
    if(fact)return {};fact=&door->Facts().physical_items[j];
   }
   if(!fact||fact->item_id!=wanted[i]||fact->units!=1)return {};
   const auto* item=c.GetInv().GetItem(fact->slot);
   if(!item||!OriginHeldActual(c,*item,fact->slot,s->mission)||
    item->GetSerialNumber()!=fact->serial||item->GetCharges()!=fact->charges)return {};
   auto token=ItemOriginToken(*item);
   RequirePublishedOrigin(database,s->mission,item->GetID(),token,false);
   for(const auto& prior:s->inputs)if(prior.facts.slot==fact->slot||prior.token==token)return {};
   s->inputs.push_back({*fact,item,std::move(token)});
  }
  return std::unique_ptr<PreparedRunActionOrigins>(new PreparedRunActionOrigins(std::move(s)));
 }catch(...){return {};}
}
bool NativeRunItemOrigins::RevalidateActionActual(Client& c,const PreparedRunActionOrigins& prepared) {
 if(!prepared.state_||prepared.state_->owner!=&c||prepared.state_->locked)return false;
 const auto& s=*prepared.state_;
 for(const auto& in:s.inputs)if(!in.actual||c.GetInv().GetItem(in.facts.slot)!=in.actual||
  !OriginHeldActual(c,*in.actual,in.facts.slot,s.mission)||in.actual->GetID()!=in.facts.item_id||
  in.actual->GetSerialNumber()!=in.facts.serial||in.actual->GetCharges()!=in.facts.charges||
  ItemOriginToken(*in.actual)!=in.token)return false;
 return true;
}
std::string NativeRunItemOrigins::LockActionActual(Database& db,uint64_t connection,
 PreparedRunActionOrigins& prepared,const OriginalDoorUse* door,const OriginalHelperUse* helper) {
 if(!prepared.state_||prepared.state_->locked||prepared.state_->inputs.empty()||
  prepared.state_->door!=door||prepared.state_->helper!=helper)
  throw std::runtime_error("Fire original action origin issuer differs");
 auto& s=*prepared.state_;
 ActionSession(db,connection);OriginFoundation(db);LockCurrentRun(db,s.mission);
 for(const auto& in:s.inputs)RequirePublishedOrigin(db,s.mission,in.facts.item_id,in.token,true);
 s.locked=true;return s.inputs.front().token;
}
void NativeRunItemOrigins::ConsumeActionActual(Database& db,uint64_t connection,
 PreparedRunActionOrigins& prepared,uint64_t action_row) {
 if(!prepared.state_||!prepared.state_->locked||prepared.state_->consumed||!action_row)
  throw std::runtime_error("Fire action origin consumption boundary differs");
 auto& s=*prepared.state_;s.consumed=true;
 ActionSession(db,connection);
 // The genuine retained VaultKey remains physically present and published1.
 if(s.kind==RunActionKind::GuardianVaultKey)return;
 for(const auto& in:s.inputs) {
  auto q=ActionQuery(db,fmt::format("UPDATE rof2_fire_run_item_origins SET state=2,consumed_at=UNIX_TIMESTAMP(),consumed_action_id={},consumed_by_character_id={} "
   "WHERE token=UNHEX('{}') AND shared_task_id={} AND item_id={} AND state=1",action_row,s.mission.character,in.token,s.mission.run,in.facts.item_id));
  if(q.RowsAffected()!=1)throw std::runtime_error("Fire action input origin consumption differs");
 }
 ActionSession(db,connection);
}
namespace {
bool OriginalLootItem(uint32_t id)noexcept {
 switch(id) {case 47748:case 47749:case 47750:case 47751:case 47752:
 case 47753:case 47754:case 47755:case 47757:case 47758:case 47760:case 47761:return true;default:return false;}
}
bool SameMission(const MissionRunFacts& a,const MissionRunFacts& b)noexcept {
 return a.run==b.run&&a.local_occurrence==b.local_occurrence&&a.character==b.character&&
  a.accepted_time==b.accepted_time&&a.zone==b.zone&&a.version==b.version&&a.instance==b.instance&&a.dynamic_zone==b.dynamic_zone;
}
bool SameOriginalLoot(const RunLootFacts& a,const RunLootFacts& b)noexcept {
 // saved_slot is supplied later by the actual positive native save observer;
 // runtime serial is audit/lifecycle concordance, never the durable item GUID.
 return SameMission(a.mission,b.mission)&&a.native_sequence==b.native_sequence&&
  a.source_death_sequence==b.source_death_sequence&&a.source_npc_type==b.source_npc_type&&
  a.source_spawn==b.source_spawn&&a.source_corpse_entity==b.source_corpse_entity&&
  a.source_loot_slot==b.source_loot_slot&&a.item_id==b.item_id&&
  a.native_item_serial==b.native_item_serial&&a.charges==b.charges;
}
}
struct PreparedRunLootSeal::State {
 std::unique_ptr<OriginalRunLootUse> original;
 std::string token;
 bool publication_taken=false;
};
PreparedRunLootSeal::PreparedRunLootSeal(std::unique_ptr<State> s)noexcept:state_(std::move(s)){}
PreparedRunLootSeal::~PreparedRunLootSeal()=default;
const std::string& PreparedRunLootSeal::Token()const noexcept{return state_->token;}
std::unique_ptr<PreparedRunLootSeal> NativeRunItemOrigins::SealOriginalLoot(
 std::unique_ptr<OriginalRunLootUse> original,EQ::ItemInstance& item) {
 try {
  if(!original||!original->state_)return {};
  const auto& f=original->Facts();
  if(!ValidMission(f.mission)||!f.native_sequence||!f.source_death_sequence||!f.source_npc_type||
   !f.source_spawn||!f.source_corpse_entity||!f.native_item_serial||f.saved_slot!=-1||
   !OriginalLootItem(f.item_id)||!OriginShape(item)||item.GetID()!=f.item_id||
   item.GetSerialNumber()!=f.native_item_serial||item.GetCharges()!=f.charges||!ItemOriginToken(item).empty())return {};
  auto s=std::make_unique<PreparedRunLootSeal::State>();
  s->token=NewOriginToken();s->original=std::move(original);
  item.SetCustomData(RunOriginSealKey,s->token);
  if(ItemOriginToken(item)!=s->token)return {};
  return std::unique_ptr<PreparedRunLootSeal>(new PreparedRunLootSeal(std::move(s)));
 }catch(...){return {};}
}
bool NativeRunItemOrigins::PublishOriginalLoot(NativeRunLootPublication& proof) {
 if(proof.taken_)return false;
 proof.taken_=true; // FIRST, including validation/query/COMMIT failure paths.
 if(!proof.state_||!proof.seal_||!proof.seal_->state_)return false;
 auto& s=*proof.seal_->state_;
 if(s.publication_taken)return false;s.publication_taken=true;
 if(!s.original||!s.original->state_||!OriginToken(s.token))return false;
 const auto p=proof.Facts();
 if(!SameOriginalLoot(p,s.original->Facts())||!ValidMission(p.mission)||!OriginalLootItem(p.item_id)||
  p.saved_slot<0)return false;
 bool transaction=false;
 try {
  // The private Source constructor owns the SAME original seal and can run
  // only after positive native Save + original corpse removal. A token, saved
  // row, actor ID or old goal alone cannot call this publication boundary.
  auto q=ActionQuery(database,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(q.RowCount()!=1||ActionNumber(q.begin()[1])||ActionNumber(q.begin()[2])!=1)return false;
  const auto connection=ActionNumber(q.begin()[0]);
  if(!database.TransactionBeginStrict().Success())return false;
  transaction=true;OriginFoundation(database);LockCurrentRun(database,p.mission);
  if(p.item_id==47758)RequireActualGuardianLoot(database,p);
  q=ActionQuery(database,fmt::format("INSERT INTO rof2_fire_run_item_origins "
   "(token,shared_task_id,item_id,origin_kind,state,producer_character_id,producer_local_occurrence,producer_accepted_time,producer_native_sequence,source_death_sequence,"
   "source_npc_type_id,source_corpse_entity_id,source_loot_slot,native_item_serial,native_item_slot,created_at,published_at,consumed_at,consumed_action_id,consumed_output_token,consumed_by_character_id) "
   "VALUES (UNHEX('{}'),{},{},1,1,{},{},{},{},{},{},{},{},{},{},UNIX_TIMESTAMP(),UNIX_TIMESTAMP(),NULL,NULL,NULL,NULL)",
   s.token,p.mission.run,p.item_id,p.mission.character,p.mission.local_occurrence,p.mission.accepted_time,p.native_sequence,
   p.source_death_sequence,p.source_npc_type,p.source_corpse_entity,p.source_loot_slot,p.native_item_serial,p.saved_slot));
  if(q.RowsAffected()!=1)throw std::runtime_error("Fire original loot origin insert differs");
  ActionSession(database,connection);const auto commit=database.TransactionCommitStrict();transaction=false;return commit.Success();
 }catch(...){if(transaction)database.TransactionRollbackStrict();return false;}
}
} // namespace FireRunActionSource
