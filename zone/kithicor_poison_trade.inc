// Included once by trading.cpp. Original accept is the sole writer entrance.
#include "inventory_session_owner.h"
#include "original_trade_offer.h"
#include "sod_food_delivery.h"
#include "original_trade_transform_hooks.h"
#include "original_message_admission.h"
#include "task_client_state.h"
#include "../common/kithicor_poison_custody.h"
#include "../common/kithicor_mission.h"
#include "../common/sod_content_bindings.h"
#include "../common/fungal_unknown_inventory.h"
#include "../common/shared_core/inventory_row_db.h"
#include "../common/shared_core/inventory_projection.h"
#include "../common/housing/evolving_item_custody_db.h"
#include "../common/eqemu_config.h"
#include <openssl/rand.h>
#include <array>
#include <set>
#include <cstring>
#include <thread>
#include <chrono>
namespace KithicorPoisonTrade {
namespace {
using namespace KithicorPoisonCustody;
using Row=EQEmu::SharedCore::InventoryRow;
using Lease=EQEmu::SharedCore::InventorySessions::Lease;
struct Ack {
 Client* client=nullptr;NPC* npc=nullptr;const EQ::ItemInstance* item=nullptr;
 uint64_t run=0;uint32_t instance=0,chest=0;std::string key;int16_t slot=-1;int32_t serial=0;int16_t charges=0;bool used=false;
};
thread_local Ack* current_ack=nullptr;
unsigned Role(NPC& npc) {
 if(!zone||zone->GetZoneID()!=457||zone->GetInstanceVersion()!=2||!zone->GetInstanceID()||!RuleB(SodProgression,EnableKithicorSlice))return 0;
 unsigned chest=0;
 for(unsigned n=1;n<=6;++n){auto type=SodContent::Role(content_db,920193,"victual_"+std::to_string(n));if(!type)continue;if(*type==npc.GetNPCTypeID()){if(chest)return 7;chest=n;}}
 return chest;
}
void Canonical(Database& db,Client& c,NPC& npc,uint64_t run,unsigned chest) {
 if(!run||!chest||chest>6||Role(npc)!=chest||!c.trade||c.trade->With()!=&npc||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||npc.GetDepop()||!c.IsTaskActivityActive(920193,10))throw std::runtime_error("Kithicor original trade changed");
 auto r=Q(db,fmt::format(
  "SELECT st.id FROM shared_tasks st JOIN shared_task_members sm ON sm.shared_task_id=st.id AND sm.character_id={} "
  "JOIN kithicor_mission_phases p ON p.shared_task_id=st.id AND p.phase=0 AND p.task_id=920193 AND p.model_version={} AND p.ready=1 AND p.scene_state=1 "
  "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=p.shared_task_id AND sd.dynamic_zone_id=p.dynamic_zone_id "
  "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 AND dz.instance_id=p.instance_id "
  "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=sm.character_id "
  "JOIN instance_list i ON i.id=dz.instance_id "
  "WHERE st.id={} AND st.task_id=920193 AND st.completion_time IS NULL AND(st.expire_time IS NULL OR st.expire_time>NOW()) "
  "AND i.id={} AND i.zone=457 AND i.version=2 AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP() FOR UPDATE",
  c.CharacterID(),KithicorMission::Model,run,zone->GetInstanceID()));
 if(r.RowCount()!=1)throw std::runtime_error("Kithicor canonical active membership changed");
}
Row Tree(ZoneDatabase& db,uint32_t who,int16_t slot,EQ::ItemInstance& item,std::set<uint32_t>& seen) {
 auto r=Q(db,"SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(slot)+" FOR UPDATE");
 if(r.RowCount()!=1||!seen.insert(uint32_t(slot)).second)throw std::runtime_error("Kithicor duplicate/missing physical row");
 auto row=EQEmu::SharedCore::InventoryRowDB::Decode(r.begin());
 if(item.GetSerialNumber()<=0||row.values.guid!=uint64_t(item.GetSerialNumber()))throw std::runtime_error("Kithicor physical GUID changed");
 row.evolving_payload=Housing::EvolvingItemCustody::Capture(db,who,slot,item);
 if(item.IsClassBag())for(const auto& entry:*item.GetContents()) {
  if(!entry.second||entry.first>=item.GetItem()->BagSlots)throw std::runtime_error("Kithicor invalid original bag tree");
  const auto child_slot=EQ::InventoryProfile::CalcSlotId(slot,entry.first);
  if(child_slot<0)throw std::runtime_error("Kithicor no durable bag destination");
  auto child=Tree(db,who,child_slot,*entry.second,seen);child.values.slot_id=entry.first;row.children.push_back(std::move(child));
 }
 if(!EQEmu::SharedCore::InventoryProjection::Matches(row,item,EQEmu::SharedCore::InventoryProjection::ReconstructionScope::CharacterProfile))throw std::runtime_error("Kithicor physical tree differs from original");
 return row;
}
bool Affected(uint32_t slot) {
 return slot==EQ::invslot::slotCursor||
  (slot>=EQ::invbag::CURSOR_BAG_BEGIN&&slot<=EQ::invbag::CURSOR_BAG_END)||
  (slot>=EQ::invslot::TRADE_BEGIN&&slot<=EQ::invslot::TRADE_NPC_END)||
  (slot>=EQ::invbag::TRADE_BAGS_BEGIN&&slot<EQ::invbag::TRADE_BAGS_BEGIN+4*EQ::invbag::SLOT_COUNT);
}
void ValidateTrees(ZoneDatabase& db,Client& c,const std::list<EQ::ItemInstance*>& queue,bool trade) {
 std::set<uint32_t> seen;size_t index=0;
 for(auto* item:queue){if(!item)throw std::runtime_error("Kithicor null cursor tree");
  const auto slot=index?EQ::invbag::CURSOR_BAG_BEGIN+index:EQ::invslot::slotCursor;
  if(index>=size_t(EQ::invbag::CURSOR_BAG_COUNT))throw std::runtime_error("Kithicor cursor capacity exhausted");
  Tree(db,c.CharacterID(),int16_t(slot),*item,seen);++index;
 }
 if(trade)for(int16_t slot=EQ::invslot::TRADE_BEGIN;slot<=EQ::invslot::TRADE_NPC_END;++slot)if(auto* item=c.GetInv().GetItem(slot))Tree(db,c.CharacterID(),slot,*item,seen);
 auto snapshot=InventoryRepository::GetLoadSnapshot(db,c.CharacterID(),true);
 if(!snapshot.success)throw std::runtime_error("Kithicor tree snapshot unavailable");
 for(const auto& row:snapshot.rows)if(Affected(row.values.slot_id)&&!seen.count(row.values.slot_id))throw std::runtime_error("Kithicor orphan/overlapping trade or cursor row");
}
std::array<uint32_t,16> Currency(const PlayerProfile_Struct& p) {
 return {{uint32_t(p.platinum),uint32_t(p.gold),uint32_t(p.silver),uint32_t(p.copper),uint32_t(p.platinum_bank),uint32_t(p.gold_bank),uint32_t(p.silver_bank),uint32_t(p.copper_bank),uint32_t(p.platinum_cursor),uint32_t(p.gold_cursor),uint32_t(p.silver_cursor),uint32_t(p.copper_cursor),p.currentRadCrystals,p.careerRadCrystals,p.currentEbonCrystals,p.careerEbonCrystals}};
}
void ValidateCurrency(ZoneDatabase& db,Client& c,const PlayerProfile_Struct& pp) {
 auto r=Q(db,"SELECT "+std::string(CurrencyColumns)+" FROM character_currency WHERE id="+std::to_string(c.CharacterID())+" FOR UPDATE");
 if(r.RowCount()!=1)throw std::runtime_error("Kithicor currency row absent");const auto fields=Currency(pp);
 for(unsigned n=0;n<16;++n)if(N(r.begin()[n])!=fields[n])throw std::runtime_error("Kithicor native currency differs");
}
void Foundation(Database& db,bool food=false,bool transform=false) {
 auto r=Q(db,transform?"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases','kithicor_poison_custody','sod_original_delivery_decisions','character_tasks','character_activities','character_task_reward_instances','shared_tasks','shared_task_members','shared_task_activity_state')":food?"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases','kithicor_poison_custody','sod_weighted_delivery_state','sod_weighted_delivery_actions','character_tasks','character_activities','character_task_reward_instances')":"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases','kithicor_poison_custody','kithicor_mission_actions','kithicor_mission_phases','shared_tasks','shared_task_members','shared_task_dynamic_zones','dynamic_zones','dynamic_zone_members','instance_list')");
 if(r.RowCount()!=1||N(r.begin()[0])!=(transform?12:food?10:13))throw std::runtime_error("Kithicor transactional foundation differs");
}
void Session(Database& db,uint64_t connection) {
 auto r=Q(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
 if(r.RowCount()!=1||N(r.begin()[0])!=connection||N(r.begin()[1])!=1||N(r.begin()[2])!=1||db.TransactionStrictFailed())throw std::runtime_error("Kithicor strict connection changed");
}
// An uncertain FIRST intent commit must not destroy the only live return plan.
// Retain the actual Client/session and resolve on fresh exclusive connections.
// If the original transaction did not commit, establish abort-only physical
// custody without claiming an action reservation or ever replaying the debit.
void EnsureAbortIntent(const Lease& lease,const std::string& key,const std::string& pre,const std::string& plan,const std::string& abort_insert,unsigned kind,const std::string& context) {
 for(;;){try{
  const auto* cfg=EQEmuConfig::get();if(!cfg)throw std::runtime_error("Kithicor configuration absent");
  Database fresh;if(!fresh.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"kithicor-intent-resolution"))throw std::runtime_error("Kithicor fresh intent connection unavailable");
  if(!fresh.TransactionBeginStrict().Success())throw std::runtime_error("Kithicor intent resolution unavailable");
  if(!Installed(fresh))throw std::runtime_error("Kithicor intent schema unavailable");LockLease(fresh,lease);
  // Locking the actual lease on this fresh connection also waits out the old
  // exclusive writer. Neither row absence nor best-effort rollback is proof.
  if(Snapshot(fresh,lease.actor.character_id)!=pre)throw std::runtime_error("Kithicor original intent preimage unresolved");
  const bool generalized=Generalized(fresh);
  if(kind!=1&&!generalized)throw std::runtime_error("Food physical schema unavailable during intent resolution");
  auto found=Q(fresh,"SELECT physical,settled,HEX(preimage),HEX(return_plan),"+std::string(generalized?"recipe_kind,HEX(recipe_context),abort_only":"1,NULL,0")+" FROM kithicor_poison_custody WHERE request_key='"+key+"' AND "+LeaseWhere(lease)+" FOR UPDATE");
  if(found.RowCount()){
   if(found.RowCount()!=1||(N(found.begin()[0])!=0&&N(found.begin()[0])!=3)||N(found.begin()[1])||!found.begin()[2]||std::string(found.begin()[2])!=Hex(pre)||!found.begin()[3]||std::string(found.begin()[3])!=Hex(plan))throw std::runtime_error("Kithicor immutable intent differs");
   if(N(found.begin()[4])!=kind||(kind==1&&found.begin()[5])||(kind!=1&&(!found.begin()[5]||std::string(found.begin()[5])!=Hex(context)||N(found.begin()[6])!=(N(found.begin()[0])==3?1:0))))throw std::runtime_error("Original trade intent recipe context differs");
  }else{
   if(KithicorPoisonCustody::Pending(fresh,lease.actor.character_id))throw std::runtime_error("Kithicor another pending intent differs");
   Q(fresh,abort_insert);
  }
  if(fresh.TransactionCommitStrict().Success())return;
 }catch(...){}std::this_thread::sleep_for(std::chrono::milliseconds(250));}
}
}

class Writer {
 struct Continuation {
  Continuation* next=nullptr;
  bool failed=false;
  std::chrono::steady_clock::time_point next_poll{};
  std::string key,pre,abort_plan;
  EQEmu::SharedCore::OriginalTradeTransform::Context context;
  Lease lease;
  const EQ::InventoryProfile* inventory=nullptr;
  FungalInventory::UnknownInventory* hold=nullptr;
 };
 inline static thread_local Continuation* continuations_=nullptr;
 static Continuation* Find(Client& c,const std::string* key=nullptr)noexcept {
  for(auto* p=continuations_;p;p=p->next)if(p->context.character==c.CharacterID()&&p->context.account==c.AccountID()&&p->inventory==&c.GetInv()&&(!key||p->key==*key))return p;
  return nullptr;
 }
public:
 static std::optional<OriginalMessageAdmission::PendingSelection> FindAdmission(Client& c,uint32_t task,uint32_t giver) noexcept {
  try {
   auto* p=Find(c);if(!p||p->failed||!EQEmu::SharedCore::OriginalTradeTransform::Admission(p->context)||p->context.task!=task||p->context.npc_entity!=giver)return std::nullopt;
   auto ticket=c.InventorySessionReleaseTicket();auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
   if(!lease||!c.HasInventorySessionReservation()||lease->epoch!=p->lease.epoch||lease->token!=p->lease.token)return std::nullopt;
   return OriginalMessageAdmission::PendingSelection{p->key,p->context};
  }catch(...){return std::nullopt;}
 }
 static void DeclineMessageTrade(Client& c) noexcept {
  auto* p=Find(c);if(!p||p->failed||!EQEmu::SharedCore::OriginalTradeTransform::Admission(p->context))return;
  try{if(OriginalMessageAdmission::DeclineUnsubmitted(c,p->key,p->context))Poll(c);}catch(...){}
 }
 static bool HasContinuation(Client& c)noexcept{return Find(c)!=nullptr;}
 static void AfterCompleteDelete(uint32_t character,uint64_t epoch)noexcept {
  auto** at=&continuations_;while(*at){auto* p=*at;if(p->context.character==character&&p->lease.epoch==epoch){*at=p->next;delete p;}else at=&p->next;}
 }
 static void Poll(Client& c)noexcept {
  auto* p=Find(c);if(!p||p->failed)return;
  const auto now=std::chrono::steady_clock::now();if(now<p->next_poll)return;
  p->next_poll=now+std::chrono::seconds(1);Resume(c,p->key);
 }
 static void Resume(Client& c,const std::string& key)noexcept {
  auto* p=Find(c,&key);if(!p||p->failed)return;
  auto* npc=entity_list.GetNPCByID(p->context.npc_entity);
  if(!npc||npc->GetNPCTypeID()!=p->context.npc_type||npc->GetSpawnPointID()!=p->context.npc_spawn){p->failed=true;c.RejectInventorySessionAdmission();c.Kick("Original delivery outcome requires custody reconciliation");return;}
  Execute(c,*npc,0,nullptr,true,p);
 }

 // Reissue only an already-settled SAME occurrence receipt, never a debit.
 // Finish this read-only exclusive transaction BEFORE global/native task calls.
 static bool ReconcilePriorFood(Client& c,NPC& npc,OriginalTradeWriter::SemanticRecipe& semantic) noexcept {
  try {
   if(!zone||zone->GetZoneID()!=452||zone->GetInstanceVersion()||zone->GetInstanceID()||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||npc.GetDepop())return false;
   auto ticket=c.InventorySessionReleaseTicket();auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
   if(!lease||!c.HasInventorySessionReservation())return false;
   auto* tasks=c.GetTaskState();if(!tasks)return false;
   int active=-1;
   for(int index=0;index<MAXACTIVEQUESTS;++index){if(tasks->GetActiveTaskID(index)==920198){if(active>=0)return false;active=index;}}
   const auto active_accepted=active>=0?tasks->GetTaskStartTime(TaskType::Quest,active):0;
   if(active<0||active_accepted<=0)return false;
   const auto* cfg=EQEmuConfig::get();if(!cfg)return false;
   ZoneDatabase db;if(!db.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"original-trade-prior-food-ack"))return false;
   if(!db.TransactionBeginStrict().Success())return false;
   if(!Generalized(db))return false;LockLease(db,*lease);
   if(KithicorPoisonCustody::Pending(db,c.CharacterID()))return false;
   auto native=Q(db,fmt::format("SELECT r.occurrence_id,r.accepted_time,t.acceptedtime,t.type,t.was_rewarded FROM character_task_reward_instances r JOIN character_tasks t ON t.charid=r.character_id AND t.taskid=r.task_id WHERE r.character_id={} AND r.task_id=920198 FOR UPDATE",c.CharacterID()));
   if(native.RowCount()!=1)return false;
   const auto occurrence=N(native.begin()[0]);const auto accepted=N(native.begin()[1]);
   if(!occurrence||accepted!=uint32_t(active_accepted)||N(native.begin()[2])!=accepted||N(native.begin()[3])!=2||N(native.begin()[4]))return false;
   // Old occurrence receipts are deliberately excluded; they cannot block or
   // grant credit to a freshly accepted personal occurrence.
   auto pending=Q(db,fmt::format("SELECT request_key FROM sod_weighted_delivery_actions WHERE character_id={} AND task_id=920198 AND occurrence_id={} AND activity_id=1 AND status=1 FOR UPDATE",c.CharacterID(),occurrence));
   if(pending.RowCount()>1)return false;
   if(!pending.RowCount())return db.TransactionCommitStrict().Success();
   if(!pending.begin()[0])return false;const std::string key=pending.begin()[0];
   // The physical row can belong to a prior inventory epoch after reconnect.
   // Actual account/character + definite physical1/settled1/context are required;
   // substituting the current lease for the old writer lease would lose retries.
   auto physical=Q(db,"SELECT HEX(recipe_context) FROM kithicor_poison_custody WHERE request_key='"+db.Escape(key)+"' AND character_id="+std::to_string(c.CharacterID())+" AND account_id="+std::to_string(c.AccountID())+" AND recipe_kind=2 AND physical=1 AND settled=1 AND abort_only=0 AND shared_task_id=0 AND phase=0 AND chest=0 FOR UPDATE");
   if(physical.RowCount()!=1||!physical.begin()[0])return false;
   const auto bytes=SodWeightedDeliveryContext::Unhex(physical.begin()[0]);
   const auto context=SodWeightedDeliveryContext::Decode(bytes);
   if(context.occurrence.character_id!=c.CharacterID()||context.occurrence.occurrence_id!=occurrence||context.occurrence.accepted_time!=accepted||context.npc.type!=npc.GetNPCTypeID())return false;
   if(SodWeightedDeliveryRecovery::Inspect(db,SodWeightedDeliveryContext::Identity(context),key,bytes)!=SodWeightedDeliveryRecovery::State::Debited)return false;
   OriginalTradeWriter::Intent intent{key,context};
   if(db.TransactionStrictFailed()||!db.TransactionCommitStrict().Success())return false;
   // No writer transaction/locks remain during native/global task persistence.
   if(!ticket->HeldFor(c)||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||npc.GetDepop())return false;
   OriginalTradeWriter::CommittedAndSettled proof(intent);
   return semantic.AcknowledgeProgress(c,npc,proof)==OriginalTradeWriter::Ack::Settled;
  }catch(...){return false;}
 }

 static bool Execute(Client& c,NPC& npc,unsigned chest,OriginalTradeWriter::SemanticRecipe* semantic=nullptr,bool transform=false,Continuation* resume=nullptr) noexcept {
  auto& inv=c.GetInv();bool transaction=false,persistence_attempted=resume!=nullptr,intent_known=resume!=nullptr;
  namespace X=EQEmu::SharedCore::OriginalTradeTransform;
  std::optional<X::Context> transform_context;
  X::Decision transform_decision;
  std::unique_ptr<Continuation> prepared_continuation;
  std::optional<FungalInventory::UnknownInventory::WriterGrant> grant;
  if(resume)grant.emplace(resume->hold);
  std::unique_ptr<FungalInventory::UnknownInventory> hold;
  std::unique_ptr<ZoneDatabase> connection_owner;
  std::optional<Lease> retained_lease;std::string retained_key,retained_pre,retained_plan,abort_insert,recipe_context;
  const unsigned kind=transform?3:semantic?2:1;
  try {
   connection_owner=std::make_unique<ZoneDatabase>();auto& db=*connection_owner;
   const auto* cfg=EQEmuConfig::get();if(!cfg||!db.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"kithicor-poison-exclusive"))return true;
   uint64_t run=semantic||transform?0:uint64_t(c.GetSharedTaskId());if(!semantic&&!transform&&c.GetSharedTaskId()<=0)return true;
   const auto canonical=[&] {
    if(transform){if(chest||!zone||zone->GetZoneID()!=452||zone->GetInstanceVersion()||zone->GetInstanceID()||!c.trade||c.trade->With()!=&npc||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||(!resume&&!OriginalTradeTransforms::ClaimsOffer(c,npc)))throw std::runtime_error("Original transform actor changed");return;}
    if(!semantic){Canonical(db,c,npc,uint64_t(run),chest);return;}
    if(chest||!zone||zone->GetZoneID()!=452||zone->GetInstanceVersion()!=0||zone->GetInstanceID()!=0||!c.trade||c.trade->With()!=&npc||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||npc.GetDepop()||!SodFoodDelivery::ClaimsOffer(c,npc))throw std::runtime_error("Food original trade changed");
   };
   canonical();
   if(c.trade->state!=Trading)return true;
   auto ticket=c.InventorySessionReleaseTicket();auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
   if(!lease||!c.HasInventorySessionReservation())return true;
   std::optional<OriginalTradeWriter::Snapshot> food_snapshot;
   if(transform) {
    if(!db.TransactionBeginStrict().Success())return true;transaction=true;
    if(!TransformSchemaReady(db))throw std::runtime_error("Original transform schema unbound");LockLease(db,*lease);
    if(resume){
     if(lease->epoch!=resume->lease.epoch||lease->token!=resume->lease.token||!ticket->HeldFor(c))throw std::runtime_error("Original continuation lease changed");
     transform_context=resume->context;
     transform_decision=OriginalTransformNative::ReadDecision(db,resume->key,*transform_context);
     if(transform_decision.state==X::DecisionState::Pending)return true;
     if(!X::Valid(*transform_context,transform_decision))throw std::runtime_error("Original shared decision differs");
    }else{
     transform_context=OriginalTradeTransforms::SnapshotOriginal(c,npc,*lease,db);
     if(!transform_context)return true;
     if(!X::Valid(*transform_context))throw std::runtime_error("Original transform snapshot unbound");
     if(!transform_context->shared){transform_decision.state=X::DecisionState::PreparedPersonal;transform_decision.counters_present=true;transform_decision.accepted[transform_context->primary]=1;transform_decision.primary_target=1;}
     else prepared_continuation=std::make_unique<Continuation>();
    }
    const auto& x=*transform_context;
    if(!X::Valid(x)||x.character!=c.CharacterID()||x.account!=c.AccountID()||x.npc_type!=npc.GetNPCTypeID()||x.npc_entity!=npc.GetID()||x.npc_spawn!=npc.GetSpawnPointID()||!EQEmu::SharedCore::OriginalTradeRecipe::Same(x.offers,OriginalTradeOffer::Capture(c)))throw std::runtime_error("Original transform captured tree differs");
    recipe_context=X::Encode(x);if(x.shared)run=x.occurrence;
   }
   if(semantic) {
    // Read-only native snapshot/locks begin before finite success-return staging.
    // No semantic/physical persistence is allowed in SnapshotOriginalOffer.
    if(!db.TransactionBeginStrict().Success())return true;transaction=true;
    if(!Generalized(db))throw std::runtime_error("Food physical schema is unbound");
    LockLease(db,*lease);
    food_snapshot=semantic->SnapshotOriginalOffer(c,npc,*lease,db);
    if(!food_snapshot)return true;
    auto& f=*food_snapshot;
    if(f.occurrence.character_id!=c.CharacterID()||f.npc.type!=npc.GetNPCTypeID()||f.npc.entity!=npc.GetID()||f.npc.spawn!=npc.GetSpawnPointID()||f.npc.zone!=zone->GetZoneID()||f.npc.version!=zone->GetInstanceVersion()||f.npc.instance!=zone->GetInstanceID()||!EQEmu::SharedCore::OriginalTradeRecipe::Same(f.offers,OriginalTradeOffer::Capture(c)))throw std::runtime_error("Food snapshot original actor differs");
    recipe_context=SodWeightedDeliveryContext::Encode(f);
   }
   // The finite planner owns no persistence or task authority. Existing leased
   // custody below remains the sole physical writer and original-accept gate.
   namespace Recipe=EQEmu::SharedCore::OriginalTradeRecipe;
   const auto offers=OriginalTradeOffer::Capture(c);
   Recipe::DebitPlan debit;
   if(transform){if(!transform_context->shared||resume)debit=X::Debit(*transform_context,transform_decision);}
   else if(semantic)debit=food_snapshot->plan.debit;
   else if(!Recipe::FirstUnit(offers,38504,debit))return true;
   if(!Recipe::Valid(offers,debit))return true;
   EQ::ItemInstance* selected=nullptr;int16_t selected_slot=-1;
   for(size_t i=0;i<Recipe::OfferCount;++i)if(debit.units[i]){
    selected_slot=offers[i].slot;selected=inv.GetItem(selected_slot);break;
   }
   if(transform){selected_slot=transform_context->offers[transform_context->primary].slot;selected=inv.GetItem(selected_slot);}
   if(!selected)return true;
   // Complete new queue owns clones. All source maps remain untouched until
   // strict COMMIT is known; list nodes and packet allocations are preflighted.
   ItemInstQueue staged;std::list<EQ::ItemInstance*> before_queue;
   for(auto it=inv.cursor_cbegin();it!=inv.cursor_cend();++it){if(!*it)return true;before_queue.push_back(*it);std::unique_ptr<EQ::ItemInstance> copy((*it)->Clone());if(!copy)return true;staged.push(copy.get());copy.release();}
   size_t abort_count=before_queue.size();
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(inv.GetItem(s))++abort_count;
   if(abort_count>size_t(EQ::invbag::CURSOR_BAG_COUNT)){c.Message(Chat::Yellow,"There is not enough durable cursor space to return your hand-in.");return true;}
   const auto populated_bag=[](const EQ::ItemInstance* p){return p&&p->IsClassBag()&&!p->GetContents()->empty();};
   for(auto* p:before_queue)if(populated_bag(p)){c.Message(Chat::Yellow,"Make room to return the offered bag and its contents safely.");return true;}
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(populated_bag(inv.GetItem(s))){c.Message(Chat::Yellow,"Make room to return the offered bag and its contents safely.");return true;}
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(auto* p=inv.GetItem(s)){
    const auto index=size_t(s-EQ::invslot::TRADE_BEGIN);
    if(!Recipe::ReturnRoot(offers[index],debit.units[index]))continue;
    std::unique_ptr<EQ::ItemInstance> copy(p->Clone());if(!copy)return true;
    copy->SetCharges(Recipe::ReturnedCharges(offers[index],debit.units[index]));staged.push(copy.get());copy.release();
   }
   int32_t output_guid=0;
   if(transform&&!transform_context->shared&&transform_context->output==X::Output::CreateBound){
    std::unique_ptr<EQ::ItemInstance> output(database.CreateItem(transform_context->output_item));
    if(!output||output->GetID()!=transform_context->output_item||output->GetSerialNumber()<=0)throw std::runtime_error("Original output definition/identity differs");
    if(output->IsStackable())output->SetCharges(1); // Exactly one bound output unit; nonstackable native default charges stay intact.
    if(c.CheckLoreConflict(output->GetItem()))throw std::runtime_error("Original output conflicts with native lore ownership");
    output_guid=output->GetSerialNumber();staged.push(output.get());output.release();
   }
   if(staged.size()>EQ::invbag::CURSOR_BAG_COUNT){c.Message(Chat::Yellow,"There is not enough durable cursor space to return your hand-in.");return true;}
   // Stock SaveCursor aliases queued roots with front-bag child positions,
   // and GetInventory reloads that range as queue roots. No populated bag has
   // a safe cursor destination here. Reject before persistence with its source
   // tree intact; never normalize away children or invent a return location.
   for(auto* p:staged)if(p->IsClassBag()&&!p->GetContents()->empty()){c.Message(Chat::Yellow,"Make room to return the offered bag and its contents safely.");return true;}
   PlayerProfile_Struct after_pp=c.GetPP();
   const auto original_currency=Currency(after_pp);for(unsigned n=0;n<12;++n)if(original_currency[n]>uint32_t(INT32_MAX))return true;
   const auto refund=[](auto& value,uint32_t amount){const auto next=int64_t(value)+amount;if(next<0||next>INT32_MAX)throw std::runtime_error("Kithicor coin refund exceeds native capacity");value=next;};
   refund(after_pp.copper,c.trade->cp);refund(after_pp.silver,c.trade->sp);refund(after_pp.gold,c.trade->gp);refund(after_pp.platinum,c.trade->pp);
   if(!resume)hold=std::unique_ptr<FungalInventory::UnknownInventory>(new FungalInventory::UnknownInventory);
   auto* active_hold=resume?resume->hold:hold.get();
   active_hold->character_=c.CharacterID();active_hold->inventory_=&inv;
   const auto remember=[&](const auto& self,const EQ::ItemInstance* p)->void {if(!p)return;active_hold->items_.push_back(p);for(const auto& child:*p->GetContents())self(self,child.second);};
   for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& entry:*bucket)remember(remember,entry.second);
   for(auto* p:before_queue)remember(remember,p);for(auto* p:staged)remember(remember,p);
   uint8_t nonce[16];if(!resume&&RAND_bytes(nonce,sizeof(nonce))!=1)return true;auto key=resume?resume->key:Hex(nonce,sizeof(nonce));if(transform&&!resume)for(auto& ch:key)if(ch>='A'&&ch<='F')ch=char(ch-'A'+'a');
   auto finish=std::make_unique<EQApplicationPacket>(OP_FinishTrade,0);
   Ack ack{&c,&npc,selected,uint64_t(run),zone->GetInstanceID(),chest,key,selected_slot,selected->GetSerialNumber(),selected->GetCharges(),false};
   OriginalTradeWriter::Intent food_intent;
   if(semantic){food_intent.request_key=key;food_intent.snapshot=*food_snapshot;}
   std::vector<EQ::ItemInstance*> delivered{selected};
   std::unique_ptr<EQApplicationPacket> cursor_packet;
   if(!staged.empty()){const auto bytes=staged.peek_front()->Serialize(EQ::invslot::slotCursor);cursor_packet=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));auto* wire=reinterpret_cast<ItemPacket_Struct*>(cursor_packet->pBuffer);wire->PacketType=ItemPacketTrade;std::memcpy(wire->SerializedItem,bytes.data(),bytes.size());}
   auto connection_row=Q(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
   if(connection_row.RowCount()!=1||N(connection_row.begin()[1])!=(transaction?1:0)||N(connection_row.begin()[2])!=1)return true;
   const auto connection=N(connection_row.begin()[0]);
   if(!transaction){if(!db.TransactionBeginStrict().Success())return true;transaction=true;}
   if(!Installed(db))throw std::runtime_error("Kithicor custody schema absent");Foundation(db,semantic!=nullptr,transform);LockLease(db,*lease);
   auto housing=Q(db,"SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(c.CharacterID())+" FOR UPDATE");
   if(housing.RowCount()>1||(housing.RowCount()&&(N(housing.begin()[0])!=c.AccountID()||(housing.begin()[1]&&*housing.begin()[1]))))throw std::runtime_error("Kithicor other inventory custody pending");
   if(!resume&&KithicorPoisonCustody::Pending(db,c.CharacterID()))throw std::runtime_error("Kithicor old custody pending");
   canonical();ValidateTrees(db,c,before_queue,true);ValidateCurrency(db,c,c.GetPP());
   if(!Recipe::Same(offers,OriginalTradeOffer::Capture(c))||!Recipe::Valid(offers,debit))throw std::runtime_error("Kithicor original offer plan changed");
   if(!semantic&&!transform){auto occupied=Q(db,fmt::format("SELECT status FROM kithicor_mission_actions WHERE shared_task_id={} AND phase=0 AND action_id={} FOR UPDATE",run,chest));
    if(occupied.RowCount())throw std::runtime_error("Kithicor chest already reserved/poisoned");}
   const auto pre=Snapshot(db,c.CharacterID());
   if(resume&&pre!=resume->pre)throw std::runtime_error("Original continuation durable preimage changed");
   // The immutable no-debit recovery plan returns the FULL original selected
   // vial as well as all other offers. It never reconstructs from a task label.
   ReturnPlan abort_plan;abort_plan.refund={{uint32_t(c.trade->pp),uint32_t(c.trade->gp),uint32_t(c.trade->sp),uint32_t(c.trade->cp)}};
   auto raw=InventoryRepository::GetLoadSnapshot(db,c.CharacterID(),true);if(!raw.success)throw std::runtime_error("Kithicor return plan source unavailable");
   if(output_guid)for(const auto& row:raw.rows)if(row.values.guid==uint64_t(output_guid))throw std::runtime_error("Original output GUID already owned");
   const auto add_return=[&](uint32_t source){
    const auto found=std::find_if(raw.rows.begin(),raw.rows.end(),[&](const auto& row){return row.values.slot_id==source;});
    if(found==raw.rows.end())throw std::runtime_error("Kithicor return plan original row absent");
    auto row=Raw(*found);const auto index=abort_plan.rows.size();row.values.slot_id=index?EQ::invbag::CURSOR_BAG_BEGIN+index:EQ::invslot::slotCursor;
    abort_plan.rows.emplace_back(source,std::move(row));
   };
   for(size_t index=0;index<before_queue.size();++index)add_return(index?EQ::invbag::CURSOR_BAG_BEGIN+index:EQ::invslot::slotCursor);
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(inv.GetItem(s))add_return(s);
   const auto abort_bytes=Encode(abort_plan);
   const bool generalized=Generalized(db);
   if(semantic&&!generalized)throw std::runtime_error("Food generalized journal absent");
   const auto insert_sql=[&](unsigned physical) {
    auto columns=std::string("request_key,character_id,account_id,inventory_epoch,inventory_token,process_key,client_key,instance_id,shared_task_id,phase,chest,npc_type_id,npc_entity_id,npc_spawn_id,source_slot,source_guid,source_charges,preimage,return_plan,postimage,physical,settled,settled_at,created_at");
    auto values=fmt::format("'{}',{},{},{},x'{}',x'{}',x'{}',{},{},0,{},{},{},{},{},{},{},x'{}',x'{}',NULL,{},0,NULL,UNIX_TIMESTAMP()",key,c.CharacterID(),c.AccountID(),lease->epoch,Hex(lease->token.data(),lease->token.size()),Hex(lease->owner.process_key),Hex(lease->owner.client_key),zone->GetInstanceID(),run,chest,npc.GetNPCTypeID(),npc.GetID(),npc.GetSpawnPointID(),selected_slot,selected->GetSerialNumber(),selected->GetCharges()>=0?selected->GetCharges():32767,Hex(pre),Hex(abort_bytes),physical);
    if(generalized){columns+=",recipe_kind,recipe_context,abort_only";values+=","+std::to_string(kind)+","+(semantic||transform?"x'"+Hex(recipe_context)+"'":"NULL")+","+std::to_string(physical==3?1:0);}
    return "INSERT INTO kithicor_poison_custody("+columns+") VALUES("+values+")";
   };
   const auto intent_insert=insert_sql(0);abort_insert=insert_sql(3);
   if(prepared_continuation){prepared_continuation->key=key;prepared_continuation->pre=pre;prepared_continuation->abort_plan=abort_bytes;prepared_continuation->context=*transform_context;prepared_continuation->lease=*lease;prepared_continuation->inventory=&inv;}
   if(!resume){
   retained_lease=*lease;retained_key=key;retained_pre=pre;retained_plan=abort_bytes;
   persistence_attempted=true;
   if(!semantic&&!transform)Q(db,fmt::format("INSERT INTO kithicor_mission_actions(shared_task_id,phase,action_id,character_id,item_id,status,updated_at) VALUES({},0,{},{},38504,0,UNIX_TIMESTAMP())",run,chest,c.CharacterID()));
   Q(db,intent_insert);
   if(transform&&!OriginalTransformNative::PrepareIntent(db,key,*transform_context))throw std::runtime_error("Original transform native intent refused");
   if(semantic&&!semantic->PrepareIntent(db,food_intent))throw std::runtime_error("Food immutable semantic intent refused");
   Session(db,connection);if(!db.TransactionCommitStrict().Success())throw std::runtime_error("Kithicor intent commit unknown");transaction=false;intent_known=true;
   if(prepared_continuation){
    // Known durable intent owns this live continuation. Inventory/coin mutation
    // is fenced until a correlated known decision; no native callbacks here.
    // Destroy unowned provisional clones before fencing. Reuse preallocated
    // vector capacity to retain only the actual live original property graph.
    while(!staged.empty())delete staged.pop();hold->items_.clear();
    for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& entry:*bucket)remember(remember,entry.second);
    for(auto* p:before_queue)remember(remember,p);
    hold->continuation_=true;hold->Attach();prepared_continuation->hold=hold.release();
    auto* pending=prepared_continuation.release();pending->next=continuations_;continuations_=pending;
    if(X::Admission(*transform_context)){
     OriginalMessageAdmission::Prepared proof(key,*transform_context);
     OriginalMessageAdmission::OpenNativeSelector(c,npc,proof);
    }else OriginalTradeTransforms::SubmitSharedSameRequest(key,*transform_context);
    return true;
   }
   if(!db.TransactionBeginStrict().Success())throw std::runtime_error("Kithicor debit transaction unavailable");transaction=true;
   } // Existing continuation resumes inside its already-open strict read tx.
   LockLease(db,*lease);canonical();
   auto intent=Q(db,"SELECT request_key FROM kithicor_poison_custody WHERE request_key='"+key+"' AND physical=0 AND settled=0 AND "+LeaseWhere(*lease)+" FOR UPDATE");
   if(intent.RowCount()!=1||Snapshot(db,c.CharacterID())!=pre||!ticket->HeldFor(c))throw std::runtime_error("Kithicor intent physical preimage changed");
   if(!Recipe::Same(offers,OriginalTradeOffer::Capture(c)))throw std::runtime_error("Original offer changed before physical debit");
   if(semantic||transform){
    auto context=Q(db,"SELECT recipe_kind,HEX(recipe_context),abort_only FROM kithicor_poison_custody WHERE request_key='"+key+"' AND "+LeaseWhere(*lease)+" FOR UPDATE");
    if(context.RowCount()!=1||N(context.begin()[0])!=kind||!context.begin()[1]||std::string(context.begin()[1])!=Hex(recipe_context)||N(context.begin()[2]))throw std::runtime_error("Food immutable physical intent differs");
   }
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(!db.SaveInventory(c.CharacterID(),nullptr,s))throw std::runtime_error("Kithicor original trade debit writer failed");
   auto empty=Q(db,fmt::format("SELECT slot_id FROM inventory WHERE character_id={} AND(slot_id BETWEEN {} AND {} OR slot_id BETWEEN {} AND {}) FOR UPDATE",c.CharacterID(),EQ::invslot::TRADE_BEGIN,EQ::invslot::TRADE_NPC_END,EQ::invbag::TRADE_BAGS_BEGIN,EQ::invbag::TRADE_BAGS_BEGIN+4*EQ::invbag::SLOT_COUNT-1));
   if(empty.RowCount())throw std::runtime_error("Kithicor original trade debit not empty");
   auto first=staged.cbegin(),last=staged.cend();if(!db.SaveCursor(c.CharacterID(),first,last))throw std::runtime_error("Kithicor original cursor writer failed");
   if(!db.SaveCharacterCurrency(c.CharacterID(),&after_pp))throw std::runtime_error("Kithicor original currency writer failed");
   const std::list<EQ::ItemInstance*> after_queue(staged.cbegin(),staged.cend());ValidateTrees(db,c,after_queue,false);ValidateCurrency(db,c,after_pp);
   auto persisted=InventoryRepository::GetLoadSnapshot(db,c.CharacterID(),true);if(!persisted.success)throw std::runtime_error("Kithicor postimage unavailable");
   std::map<uint32_t,InventoryRepository::LoadFields> untouched;
   for(const auto& row:raw.rows)if(!Affected(row.values.slot_id))untouched.emplace(row.values.slot_id,row.fields);
   size_t unchanged=0;for(const auto& row:persisted.rows)if(!Affected(row.values.slot_id)){auto found=untouched.find(row.values.slot_id);if(found==untouched.end()||found->second!=row.fields)throw std::runtime_error("Kithicor unrelated inventory changed");++unchanged;}
   if(unchanged!=untouched.size())throw std::runtime_error("Kithicor unrelated inventory omitted");
   const auto post=Snapshot(db,c.CharacterID());
   // This semantic CAS shares this exact exclusive physical transaction. No
   // semantic hook can COMMIT independently; Session() below verifies ownership.
   if(transform&&!OriginalTransformNative::CommitSettlement(db,key,*transform_context,transform_decision))throw std::runtime_error("Original native output settlement refused");
   if(semantic&&!semantic->CommitDebitAndProgress(db,food_intent))throw std::runtime_error("Food physical/semantic CAS refused");
   auto outcome=Q(db,"UPDATE kithicor_poison_custody SET physical=1,postimage=x'"+Hex(post)+"' WHERE request_key='"+key+"' AND physical=0 AND settled=0 AND "+LeaseWhere(*lease));
   if(outcome.RowsAffected()!=1||db.TransactionStrictFailed())throw std::runtime_error("Kithicor physical outcome changed");
   Session(db,connection);if(!db.TransactionCommitStrict().Success())throw std::runtime_error("Kithicor physical commit unknown");transaction=false;
   // Allocation-free native ownership publication. staged now owns old cursor.
   inv.m_cursor.Swap(staged);std::array<std::unique_ptr<EQ::ItemInstance>,4> spent;
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s){auto at=inv.m_trade.find(s);if(at!=inv.m_trade.end()){spent[s-EQ::invslot::TRADE_BEGIN].reset(at->second);at->second=nullptr;}}
   c.GetPP()=after_pp;
   auto settled=Q(db,"UPDATE kithicor_poison_custody SET settled=1,settled_at=UNIX_TIMESTAMP() WHERE request_key='"+key+"' AND physical=1 AND settled=0 AND "+LeaseWhere(*lease));
   if(settled.RowsAffected()!=1)throw std::runtime_error("Kithicor memory publication acknowledgment unavailable");
   // New one-use gameplay proof is issued ONLY after definite physical commit
   // and native inventory ownership settlement; the same writer serves both.
   if(resume){
    // Old staged/cursor/source ownership is no longer live. Dispose under the
    // exact private writer grant before invoking any semantic callback.
    while(!staged.empty())delete staged.pop();for(auto& old:spent)old.reset();
    active_hold->items_.clear();
    for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& entry:*bucket)remember(remember,entry.second);
    for(auto it=inv.cursor_cbegin();it!=inv.cursor_cend();++it)remember(remember,*it);
   }
   grant.reset(); // No writer inventory bypass during native callbacks.
   if(transform){
    OriginalTradeTransforms::Committed proof(key,*transform_context);
    if(!OriginalTradeTransforms::Acknowledge(c,proof))throw std::runtime_error("Original transform native acknowledgment pending");
   }else if(semantic) {
    OriginalTradeWriter::CommittedAndSettled proof(food_intent);
    if(semantic->AcknowledgeProgress(c,npc,proof)!=OriginalTradeWriter::Ack::Settled)throw std::runtime_error("Food durable progress awaits native reconciliation");
   } else {
    Canonical(db,c,npc,uint64_t(run),chest);
    struct Scope {Ack* old;Scope(Ack& a):old(current_ack){current_ack=&a;}~Scope(){current_ack=old;}} scope(ack);
    const bool updated=c.UpdateTasksOnDeliver(delivered,*c.trade,&npc);
    bool gameplay_accepted=false;
    try{auto receipt=Q(db,fmt::format("SELECT status FROM kithicor_mission_actions WHERE shared_task_id={} AND phase=0 AND action_id={} AND character_id={} AND item_id=38504 AND status=1",run,chest,c.CharacterID()));gameplay_accepted=updated&&receipt.RowCount()==1;}catch(...){}
    if(!gameplay_accepted)c.Message(Chat::Yellow,"Your vial was used, but task progress could not be confirmed.");
   }
   c.trade->Reset();c.QueuePacket(finish.get());if(cursor_packet)c.QueuePacket(cursor_packet.get());c.SendMoneyUpdate();c.RecalcWeight();
   if(resume){auto* h=resume->hold;const auto epoch=resume->lease.epoch;h->ReleaseKnownContinuation();AfterCompleteDelete(c.CharacterID(),epoch);}
   return true;
  }catch(...) {
   if(transaction&&connection_owner)connection_owner->TransactionRollbackStrict(); // NO inference of known rollback.
   connection_owner.reset(); // End this exclusive writer before fresh observation.
   grant.reset();
   if(persistence_attempted&&(hold||resume||Find(c))){
    if(hold){hold->Attach();hold.release();}
    if(!intent_known&&retained_lease)EnsureAbortIntent(*retained_lease,retained_key,retained_pre,retained_plan,abort_insert,kind,recipe_context);
    c.RejectInventorySessionAdmission();c.Kick("Kithicor hand-in custody requires reconciliation");
   }
   // Before any persistence attempt the native maps and durable property are
   // untouched. After an attempted write, teardown/recovery owns reconciliation.
   return true;
  }
 }
};
bool HasVial(Client& c) {
 for(int16_t slot=EQ::invslot::TRADE_BEGIN;slot<=EQ::invslot::TRADE_NPC_END;++slot)if(auto* item=c.GetInv().GetItem(slot);item&&item->GetID()==38504)return true;
 return false;
}
bool RejectUnissued(Client& c,NPC& npc){return HasVial(c)&&Role(npc)!=0;}
bool OriginalAccept(Client& c,NPC& npc){if(!HasVial(c))return false;const auto chest=Role(npc);if(!chest)return false;return Writer::Execute(c,npc,chest);}
bool ConsumeAcknowledgment(Client& c,NPC& npc,const EQ::ItemInstance& item) {
 auto* a=current_ack;if(!a||a->used||a->client!=&c||a->npc!=&npc||a->item!=&item||item.GetID()!=38504||item.GetSerialNumber()!=a->serial||item.GetCharges()!=a->charges||c.GetSharedTaskId()!=int64_t(a->run)||!zone||zone->GetInstanceID()!=a->instance||Role(npc)!=a->chest)return false;
 try{
  auto ticket=c.InventorySessionReleaseTicket();auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
  if(!lease||!c.HasInventorySessionReservation())return false;
  LockLease(database,*lease);Canonical(database,c,npc,a->run,a->chest);
  auto r=Q(database,fmt::format("SELECT request_key FROM kithicor_poison_custody WHERE request_key='{}' AND physical=1 AND settled=1 AND {} AND shared_task_id={} AND phase=0 AND chest={} AND source_slot={} AND source_guid={} AND source_charges={} AND npc_type_id={} AND npc_entity_id={} AND npc_spawn_id={}",a->key,LeaseWhere(*lease),a->run,a->chest,a->slot,a->serial,a->charges>=0?a->charges:32767,npc.GetNPCTypeID(),npc.GetID(),npc.GetSpawnPointID()));
  if(r.RowCount()!=1)return false;a->used=true;return true;
 }catch(...){return false;}
}
}

namespace OriginalTradeFood {
bool OriginalAccept(Client& c,NPC& npc) {
 try{if(!SodFoodDelivery::ClaimsOffer(c,npc))return false;
  auto& semantic=SodFoodDelivery::RecipeHooks();
  if(!KithicorPoisonTrade::Writer::ReconcilePriorFood(c,npc,semantic))return true;
  return KithicorPoisonTrade::Writer::Execute(c,npc,0,&semantic);
 }catch(...){return true;}
}
bool RejectUnissued(Client& c,NPC& npc){try{return SodFoodDelivery::ClaimsOffer(c,npc);}catch(...){return true;}}
}

namespace OriginalTradeTransforms {
bool OriginalAccept(Client& c,NPC& npc){if(Pending(c)){PollPending(c);return true;}try{if(!ClaimsOffer(c,npc))return false;return KithicorPoisonTrade::Writer::Execute(c,npc,0,nullptr,true);}catch(...){return true;}}
bool Pending(Client& c){return KithicorPoisonTrade::Writer::HasContinuation(c);}
void PollPending(Client& c)noexcept{KithicorPoisonTrade::Writer::Poll(c);}
void ResumeKnownDecision(Client& c,const std::string& key){KithicorPoisonTrade::Writer::Resume(c,key);}
void AfterCompleteDelete(uint32_t character,uint64_t epoch)noexcept{KithicorPoisonTrade::Writer::AfterCompleteDelete(character,epoch);}
}

namespace OriginalMessageAdmission {
std::optional<PendingSelection> FindForNativeSelection(Client& c,uint32_t task,uint32_t giver){return KithicorPoisonTrade::Writer::FindAdmission(c,task,giver);}
}
namespace OriginalTradeTransforms {
void DeclineOriginalMessageTrade(Client& c)noexcept{KithicorPoisonTrade::Writer::DeclineMessageTrade(c);}
}
