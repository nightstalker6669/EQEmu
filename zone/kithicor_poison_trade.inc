// Included once by trading.cpp. Original accept is the sole writer entrance.
#include "inventory_session_owner.h"
#include "dodh_rakban_combine.h"
#include "common/evolving_items.h"
#include "common/repositories/character_evolving_items_repository.h"
#include "string_ids.h"
#include "original_trade_offer.h"
#include "sod_food_delivery.h"
#include "original_trade_transform_hooks.h"
#include "original_message_admission.h"
#include "dodh_exchange.h"
#include "dodh_mission_custody.h"
#include "dodh_longshadow_source.h"
#include "common/shared_core/dodh_longshadow_sources.h"
#include "common/shared_core/dodh_longshadow_journal.h"
#include "dynamic_zone.h"
#include "dodh_spell_hail.h"
#include "dodh_spell_reward.h"
#include "dodh_rakban_claim.h"
#include "../common/shared_task_admission.h"
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
#include "dodh_longshadow_scene.inc"
#include "dodh_longshadow_travel.inc"
#include "dodh_hard_notebook_chest.inc"
#include "dodh_bellfast_hive.inc"
#include "dodh_bellfast_korlach.inc"
#include "dodh_bellfast_neran.inc"
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
namespace Longshadow = EQEmu::SharedCore::DodhLongshadowCustody;

const Longshadow::Route* MatchingLongshadowOffer(Client& c, NPC& npc)
{
 if (!zone || c.GetSharedTaskId() <= 0 || !c.GetTaskState()) return nullptr;
 const auto task = c.GetTaskState()->GetActiveSharedTask().task_id;
 const auto offers = OriginalTradeOffer::Capture(c);
 const Longshadow::Route* matched = nullptr;
 for (const auto& route : Longshadow::NormalRoutes) {
  if (route.task != task || route.zone != zone->GetZoneID() || route.version != zone->GetInstanceVersion() ||
      route.npc != npc.GetNPCTypeID() || route.spawn != npc.GetSpawnPointID() ||
      bool(zone->GetInstanceID()) != (route.zone != 358)) continue;
  if (std::any_of(offers.begin(), offers.end(), [&](const auto& offer) {
      return offer.present && offer.item_id == route.item;
  })) {
   if (c.IsTaskActivityActive(route.task, route.activity)) return &route;
   if (!matched) matched = &route;
  }
 }
 return matched;
}

std::optional<Longshadow::Context> BindLongshadow(
 ZoneDatabase& db, Client& c, NPC& npc, const Longshadow::Route& route)
{
 if (MatchingLongshadowOffer(c, npc) != &route ||
     !c.IsTaskActivityActive(route.task, route.activity)) return std::nullopt;
 const auto placement = DodhMissionCustody::Placement(route.task);
 if (!placement) return std::nullopt;
 const auto run = uint64_t(c.GetSharedTaskId());
 auto rows = Q(db, fmt::format(
     "SELECT UNIX_TIMESTAMP(live.accepted_time),i.id FROM shared_tasks live "
     "JOIN shared_task_members m ON m.shared_task_id=live.id AND m.character_id={} "
     "JOIN shared_task_dynamic_zones link ON link.shared_task_id=live.id "
     "JOIN dynamic_zones dz ON dz.id=link.dynamic_zone_id AND dz.type=4 "
     "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=m.character_id "
     "JOIN instance_list i ON i.id=dz.instance_id AND i.zone={} AND i.version={} "
     "WHERE live.id={} AND live.task_id={} AND live.completion_time IS NULL "
     "AND (live.expire_time IS NULL OR live.expire_time>NOW())",
     c.CharacterID(), placement->zone, placement->version, run, route.task));
 if (rows.RowCount() != 1) return std::nullopt;
 const auto accepted = N(rows.begin()[0]), instance = N(rows.begin()[1]);
 if (!accepted || accepted > UINT32_MAX || !instance || instance > UINT32_MAX) return std::nullopt;
 return Longshadow::Context{run, uint32_t(accepted), c.CharacterID(), route.task,
     route.activity, uint32_t(instance), route.zone, route.version, zone->GetInstanceID(),
     npc.GetNPCTypeID(), npc.GetSpawnPointID(), true};
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

 enum class CursorResult { Refused, Issued, AlreadyIssued, InventoryReload };

 static CursorResult ClaimDodhCursor(Client& c, std::optional<DodhSpellReward::Route> spell_route,
     const Longshadow::SourceRoute* source_route = nullptr, const EQ::ItemInstance* native_loot = nullptr,
     unsigned rakban_route = 0, const DodhLongshadowScene::Identity* scene = nullptr,
     uint64_t notebook_run = 0, bool clicktok_hail = false) noexcept {
  const bool chest = (scene && !source_route) || notebook_run;
  if (clicktok_hail && (!source_route || source_route->source != Longshadow::Source::ClicktokGem ||
      native_loot || chest)) return CursorResult::Refused;
  const bool physical_source = native_loot || clicktok_hail;
  if ((notebook_run && (source_route || scene)) ||
      ((source_route || scene || notebook_run) && (spell_route || rakban_route)) ||
      (!source_route && !scene && !notebook_run && spell_route.has_value() == (rakban_route != 0)) ||
      ((scene || notebook_run) && !native_loot) ||
      rakban_route > DodhRakbanClaim::kShroudClaim) return CursorResult::Refused;
  const auto* claim_table = source_route ? "dodh_longshadow_item_sources"
      : notebook_run ? "dodh_hard_notebook_chests"
      : chest ? "dodh_longshadow_scene_effects"
      : spell_route ? DodhSpellReward::ClaimTable(*spell_route) : "dodh_rakban_claims";
  if (!claim_table) return CursorResult::Refused;
  const auto route_id = spell_route ? static_cast<unsigned>(*spell_route) : rakban_route;
  auto& inv = c.GetInv();
  if (c.IsSpiritShrouded() || !FungalInventory::UnknownInventory::ProfileAllowed(&inv) || !c.trade ||
      c.trade->state != TradeNone || c.trade->With()) return CursorResult::Refused;

  auto ticket = c.InventorySessionReleaseTicket();
  auto lease = ticket ? ticket->SnapshotFor(c) : std::nullopt;
  if (!lease || !c.HasInventorySessionReservation()) return CursorResult::Refused;

  std::unique_ptr<FungalInventory::UnknownInventory> hold;
  bool commit_unknown = false;
  try {
   ZoneDatabase db;
   const auto* cfg = EQEmuConfig::get();
   if (!cfg || !db.Connect(cfg->DatabaseHost, cfg->DatabaseUsername, cfg->DatabasePassword,
       cfg->DatabaseDB, cfg->DatabasePort, "dodh-hail-exclusive") ||
       !db.TransactionBeginStrict().Success()) return CursorResult::Refused;
   struct Rollback {
    ZoneDatabase& db;
    ~Rollback() { db.TransactionRollbackStrict(); }
   } rollback{db};

   LockLease(db, *lease);
   const auto who = c.CharacterID();
   auto engines = Q(db, fmt::format(
       "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
       "AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases','{}')",
       claim_table));
   if (engines.RowCount() != 1 || N(engines.begin()[0]) != 5) return CursorResult::Refused;
   auto housing = Q(db, fmt::format(
       "SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id={} FOR UPDATE", who));
   if (housing.RowCount() > 1 || (housing.RowCount() &&
       (N(housing.begin()[0]) != c.AccountID() || (housing.begin()[1] && *housing.begin()[1])))) return CursorResult::Refused;
   if (KithicorPoisonCustody::Pending(db, who)) return CursorResult::Refused;

   std::optional<Longshadow::SourceReceipt> source;
   std::string receipt_sql;
   const auto base_class = c.GetBaseClass();
   DodhSpellReward::ClassReward reward{};
   std::string scene_scope;
   if (scene) {
    const auto actor = chest ? 363035u : 354920u;
    const auto* route = DodhLongshadowScene::RouteForActor(actor);
    if (!route || !DodhLongshadowScene::InScope(*route) || !DodhLongshadowScene::Member(c, *scene) ||
        !Longshadow::SceneLoot(actor, native_loot->GetID()) ||
        (!chest && source_route->source != Longshadow::Source::ShadowspinCorpse)) return CursorResult::Refused;
    scene_scope = DodhLongshadowScene::ExactScope(*route, *scene);
    auto terminal = Q(db, fmt::format(
        "SELECT e.terminal FROM dodh_longshadow_scene_effects e{}WHERE {} AND {} "
        "AND e.materialized=1 AND e.death_character_id IS NOT NULL AND e.loot_item_id={} FOR UPDATE",
        DodhLongshadowScene::ScopeJoin, scene_scope, DodhLongshadowScene::MemberWhere(who), native_loot->GetID()));
    if (terminal.RowCount() != 1) return CursorResult::Refused;
    if (N(terminal.begin()[0]) == 1) return CursorResult::AlreadyIssued;
   }
   std::string notebook_scope;
   if (notebook_run) {
    namespace Notebook = DodhHardNotebookChest;
    notebook_scope = Notebook::LootScope(c, notebook_run, native_loot->GetID());
    if (notebook_scope.empty()) return CursorResult::Refused;
    auto terminal = Q(db, fmt::format("SELECT c.loot_guid{}{}{} FOR UPDATE",
        Notebook::LootJoin, Notebook::ArchiveJoin, notebook_scope));
    if (terminal.RowCount() != 1) return CursorResult::Refused;
    if (N(terminal.begin()[0])) return CursorResult::AlreadyIssued;
    reward.item_id = native_loot->GetID();
    receipt_sql = fmt::format("SELECT c.looted_by,c.item_id,0,c.loot_guid{}{}{} AND c.loot_guid IS NOT NULL FOR UPDATE",
        Notebook::LootJoin, Notebook::ArchiveJoin, notebook_scope);
   } else if (source_route) {
    const auto placement = DodhMissionCustody::Placement(source_route->task);
    const auto* kind = DodhMissionCustody::SourceKind(source_route->source);
    if (!placement || !kind || c.GetSharedTaskId() <= 0 ||
        !(source_route->origin == Longshadow::Origin::Acceptance ? c.IsTaskActive(source_route->task) :
          c.IsTaskActivityActive(source_route->task, source_route->activity)) ||
        !zone || (source_route->origin == Longshadow::Origin::Corpse ||
                  source_route->origin == Longshadow::Origin::Ground) != physical_source)
     return CursorResult::Refused;
    const auto run = uint64_t(c.GetSharedTaskId());
    auto context = Q(db, fmt::format(
        "SELECT UNIX_TIMESTAMP(live.accepted_time),i.id FROM shared_tasks live "
        "JOIN shared_task_members m ON m.shared_task_id=live.id AND m.character_id={} "
        "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=live.id "
        "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 "
        "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=m.character_id "
        "JOIN instance_list i ON i.id=dz.instance_id AND i.zone={} AND i.version={} "
        "WHERE live.id={} AND live.task_id={} AND live.completion_time IS NULL "
        "AND (live.expire_time IS NULL OR live.expire_time>NOW()){} FOR UPDATE",
        who, placement->zone, placement->version, run, source_route->task,
        source_route->origin == Longshadow::Origin::Hail ? " AND m.is_leader=1" : ""));
    if (context.RowCount() != 1) return CursorResult::Refused;
    const auto accepted = N(context.begin()[0]), instance = N(context.begin()[1]);
    if (!accepted || accepted > UINT32_MAX || !instance || instance > UINT32_MAX ||
        bool(zone->GetInstanceID()) != physical_source ||
        (physical_source && zone->GetInstanceID() != instance)) return CursorResult::Refused;

    if (clicktok_hail) {
     auto meat = Q(db, fmt::format(
         "SELECT i.slot_id,i.guid FROM dodh_longshadow_item_sources s "
         "JOIN inventory i ON i.character_id=s.character_id AND i.item_id=s.item_id "
         "AND i.guid=COALESCE(s.inventory_guid,s.item_guid) "
         "WHERE s.shared_task_id={} AND s.accepted_time={} AND s.character_id={} "
         "AND s.task_id=920270 AND s.instance_id={} AND s.item_id=87014 "
         "AND s.source_kind='bellfast_alpha_larva_meat' AND "
         "(i.slot_id BETWEEN {} AND {} OR i.slot_id BETWEEN {} AND {} OR i.slot_id={}) FOR UPDATE",
         run, accepted, who, instance, EQ::invslot::GENERAL_BEGIN, EQ::invslot::GENERAL_END,
         EQ::invbag::GENERAL_BAGS_BEGIN, EQ::invbag::GENERAL_BAGS_END, EQ::invslot::slotCursor));
     if (meat.RowCount() != 1) return CursorResult::Refused;
     const auto* carried = inv.GetItem(static_cast<int16>(N(meat.begin()[0])));
     if (!carried || carried->GetID() != 87014 || carried->GetSerialNumber() <= 0 ||
         uint64_t(carried->GetSerialNumber()) != N(meat.begin()[1])) return CursorResult::Refused;
    }
    source = Longshadow::SourceReceipt{run, uint32_t(accepted), who, source_route->task,
        uint32_t(instance), source_route->item, 0, source_route->source};
    if (source_route->source == Longshadow::Source::ZlixtilGem &&
        !DodhBellfastHive::LootWitness(db, c, uint32_t(accepted), uint32_t(instance)))
     return CursorResult::Refused;
    if (source_route->source == Longshadow::Source::TraitorChest) {
     const auto opened = Q(db, fmt::format(
         "SELECT 1 FROM dodh_longshadow_traitor_chests WHERE shared_task_id={} "
         "AND accepted_time={} AND instance_id={} AND opened_by IS NOT NULL FOR UPDATE",
         run, accepted, instance));
     if (opened.RowCount() != 1) return CursorResult::Refused;
    }
    if (source_route->origin == Longshadow::Origin::Acceptance) {
     auto event = Q(db, fmt::format(
         "SELECT 1 FROM dodh_longshadow_note_events WHERE shared_task_id={} AND character_id={} "
         "AND accepted_time={} FOR UPDATE", run, who, accepted));
     if (event.RowCount() != 1) return CursorResult::Refused;
    }
    receipt_sql = fmt::format(
        "SELECT character_id,item_id,accepted_time,COALESCE(inventory_guid,item_guid),task_id,instance_id "
        "FROM dodh_longshadow_item_sources WHERE shared_task_id={} AND source_kind='{}' FOR UPDATE",
        run, kind);
    reward.item_id = source_route->item;
   } else if (chest) {
    reward.item_id = native_loot->GetID();
    receipt_sql = fmt::format(
        "SELECT loot_character_id,loot_item_id,0,loot_item_guid FROM dodh_longshadow_scene_effects "
        "WHERE request_key='{}' AND terminal=1 FOR UPDATE", scene->key);
   } else {
    receipt_sql = fmt::format(
        "SELECT class_id,item_id,spell_id,item_serial FROM {} "
        "WHERE character_id={} AND route_id={} FOR UPDATE", claim_table, who, route_id);
   }
   auto existing = Q(db, receipt_sql);
   if (existing.RowCount()) {
    auto row = existing.begin();
    if (source && existing.RowCount() == 1 && N(row[0]) &&
        N(row[1]) == source->item && N(row[2]) == source->accepted_time && N(row[3]) &&
        N(row[4]) == source->task && N(row[5]) == source->mission_instance)
     return CursorResult::AlreadyIssued;
    return CursorResult::Refused;
   }
   if (!source && !chest) {
    const auto has_completion = [&](uint32_t task) {
     if (spell_route && *spell_route == DodhSpellReward::Route::Level69) {
      return SharedTaskAdmission::CompletedMember(db, task, {who}) == who;
     }
     const auto completed = Q(db, fmt::format(
         "SELECT 1 FROM completed_shared_tasks occurrence "
         "JOIN completed_shared_task_members member ON member.shared_task_id=occurrence.id "
         "WHERE occurrence.task_id={} AND member.character_id={} "
         "AND occurrence.completion_time IS NOT NULL AND {} LIMIT 1", task, who,
         spell_route ? "1=1" : "UNIX_TIMESTAMP(occurrence.completion_time)>0"));
     return completed.RowCount() == 1;
    };
    const auto already_claimed = [&](unsigned slot) {
     const auto paid = Q(db, fmt::format(
         "SELECT 1 FROM dodh_rakban_claims WHERE character_id={} AND route_id={} "
         "AND item_id={} AND spell_id=0 AND item_serial>0 FOR UPDATE", who, slot, 85565 + slot));
     return paid.RowCount() == 1;
    };
    reward = spell_route
        ? DodhSpellReward::Resolve(*spell_route, base_class, has_completion)
        : DodhSpellReward::ClassReward{
            DodhRakbanClaim::Resolve(rakban_route, has_completion, already_claimed), 0};
   }
   if (!reward.item_id || inv.CursorSize() >= EQ::invbag::CURSOR_BAG_COUNT) return CursorResult::Refused;
   std::unique_ptr<EQ::ItemInstance> output(native_loot ? native_loot->Clone() : database.CreateItem(reward.item_id));
   if (!output || output->GetID() != reward.item_id || output->GetSerialNumber() <= 0 ||
       (spell_route && output->GetItem()->Scroll.Effect != reward.spell_id) ||
       ((source || chest) && (output->IsStackable() || output->IsClassBag())) ||
       c.CheckLoreConflict(output->GetItem())) return CursorResult::Refused;
   if (output->IsStackable()) output->SetCharges(1);

   std::list<EQ::ItemInstance*> before;
   ItemInstQueue staged;
   for (auto it = inv.cursor_cbegin(); it != inv.cursor_cend(); ++it) {
    auto* item = *it;
    if (!item || (item->IsClassBag() && !item->GetContents()->empty())) return CursorResult::Refused;
    std::unique_ptr<EQ::ItemInstance> copy(item->Clone());
    if (!copy) return CursorResult::Refused;
    before.push_back(item);
    staged.push(copy.get());
    copy.release();
   }
   ValidateTrees(db, c, before, false);
   auto raw = InventoryRepository::GetLoadSnapshot(db, who, true);
   if (!raw.success) return CursorResult::Refused;
   const auto serial = output->GetSerialNumber();
   for (const auto& row : raw.rows) {
    if (row.values.guid == uint64_t(serial)) return CursorResult::Refused;
   }

   auto* scroll = output.get();
   staged.push(scroll);
   output.release();
   std::unique_ptr<EQApplicationPacket> cursor_packet;
   if (before.empty()) {
    const auto bytes = scroll->Serialize(EQ::invslot::slotCursor);
    cursor_packet = std::make_unique<EQApplicationPacket>(OP_ItemPacket, bytes.size() + sizeof(ItemPacket_Struct));
    auto* wire = reinterpret_cast<ItemPacket_Struct*>(cursor_packet->pBuffer);
    wire->PacketType = ItemPacketLimbo;
    std::memcpy(wire->SerializedItem, bytes.data(), bytes.size());
   }
   hold.reset(new FungalInventory::UnknownInventory);
   hold->character_ = who;
   hold->inventory_ = &inv;
   const auto remember = [&](const auto& self, const EQ::ItemInstance* item) -> void {
    if (!item) return;
    hold->items_.push_back(item);
    for (const auto& child : *item->GetContents()) self(self, child.second);
   };
   for (const auto* bucket : {&inv.m_worn, &inv.m_inv, &inv.m_bank, &inv.m_shbank, &inv.m_trade}) {
    for (const auto& entry : *bucket) remember(remember, entry.second);
   }
   for (const auto* item : before) remember(remember, item);

   const auto pre = Snapshot(db, who);
   auto first = staged.cbegin(), last = staged.cend();
   if (!db.SaveCursor(who, first, last)) return CursorResult::Refused;
   ValidateTrees(db, c, std::list<EQ::ItemInstance*>(staged.cbegin(), staged.cend()), false);
   if (source) {
    source->guid = serial;
    if (!DodhMissionCustody::RecordSource(db, *source)) return CursorResult::Refused;
   } else if (!chest) {
    const auto claim = Q(db, fmt::format(
        "INSERT INTO {} (character_id,route_id,class_id,item_id,spell_id,item_serial) "
        "VALUES ({},{},{},{},{},{})", claim_table, who, route_id, base_class, reward.item_id, reward.spell_id, serial));
    if (claim.RowsAffected() != 1) return CursorResult::Refused;
   }
   if (notebook_run) {
    const auto claim = Q(db, fmt::format(
        "UPDATE dodh_hard_notebook_chests c JOIN shared_tasks live ON live.id=c.shared_task_id{} "
        "SET c.looted_by={},c.loot_guid={} {} AND c.loot_guid IS NULL",
        DodhHardNotebookChest::ArchiveJoin, who, serial, notebook_scope));
    if (claim.RowsAffected() != 1) return CursorResult::Refused;
   }
   if (scene) {
    const auto claim = Q(db, fmt::format(
        "UPDATE dodh_longshadow_scene_effects e{}SET e.terminal=1,e.loot_character_id={},e.loot_item_guid={} "
        "WHERE {} AND {} AND e.materialized=1 AND e.terminal=0 "
        "AND e.death_character_id IS NOT NULL AND e.loot_item_id={}",
        DodhLongshadowScene::ScopeJoin, who, serial, scene_scope,
        DodhLongshadowScene::MemberWhere(who), reward.item_id));
    if (claim.RowsAffected() != 1) return CursorResult::Refused;
   }
   if (db.TransactionStrictFailed() || !ticket->HeldFor(c)) return CursorResult::Refused;

   const auto post = Snapshot(db, who);
   commit_unknown = true;
   if (!db.TransactionCommitStrict().Success()) {
    ZoneDatabase readback;
    if (!readback.Connect(cfg->DatabaseHost, cfg->DatabaseUsername, cfg->DatabasePassword,
        cfg->DatabaseDB, cfg->DatabasePort, "dodh-hail-readback") ||
        !readback.TransactionBeginStrict().Success()) throw std::runtime_error("Darkhollow claim readback unavailable");
    Rollback readback_rollback{readback};
    LockLease(readback, *lease);
    auto receipt = Q(readback, receipt_sql);
    const auto actual = Snapshot(readback, who);
    if (!receipt.RowCount() && actual == pre) {
     commit_unknown = false;
     return CursorResult::Refused;
    }
    if (receipt.RowCount() != 1 || actual != post || !ticket->HeldFor(c)) {
     throw std::runtime_error("Darkhollow claim outcome unknown");
    }
    auto row = receipt.begin();
    if (N(row[0]) != (source || chest ? uint64_t(who) : uint64_t(base_class)) || N(row[1]) != reward.item_id ||
        N(row[2]) != (source ? source->accepted_time : reward.spell_id) || N(row[3]) != uint64_t(serial) ||
        (source && (N(row[4]) != source->task || N(row[5]) != source->mission_instance))) {
     throw std::runtime_error("Darkhollow claim receipt differs");
    }
   }
   commit_unknown = false;
   inv.m_cursor.Swap(staged);
   if (cursor_packet) c.QueuePacket(cursor_packet.get());
   c.RecalcWeight();
   return CursorResult::Issued;
  } catch (...) {
   if (!commit_unknown) return CursorResult::Refused;
   hold->Attach();
   hold.release();
   c.RejectInventorySessionAdmission();
   c.Kick("Darkhollow item claim requires inventory reload");
   return CursorResult::InventoryReload;
  }
 }

 static bool ClaimDodhHailItem(Client& c, std::optional<DodhSpellReward::Route> spell_route,
     unsigned rakban_route = 0, bool* delivered = nullptr) noexcept {
  if (delivered) *delivered = false;
  const auto result = ClaimDodhCursor(c, spell_route, nullptr, nullptr, rakban_route);
  if (delivered) *delivered = result == CursorResult::Issued;
  return result == CursorResult::InventoryReload;
 }

#include "dodh_rakban_combine.inc"

 enum class DodhTrade { None, Exchange, DesertingGuard, NotebookOpener, Monocle, PublicRoute, Longshadow };

 static bool Execute(Client& c,NPC& npc,unsigned chest,OriginalTradeWriter::SemanticRecipe* semantic=nullptr,bool transform=false,Continuation* resume=nullptr,DodhTrade dodh_trade=DodhTrade::None,
                     const EQEmu::SharedCore::DodhExchange::PublicRoute* route=nullptr) noexcept {
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
  namespace Exchange = EQEmu::SharedCore::DodhExchange;
  const bool dodh = dodh_trade != DodhTrade::None;
  const bool dodh_monocle = dodh_trade == DodhTrade::Monocle;
  const bool dodh_exchange = dodh_trade == DodhTrade::Exchange || dodh_monocle;
  const bool dodh_guard = dodh_trade == DodhTrade::DesertingGuard;
  const bool dodh_opener = dodh_trade == DodhTrade::NotebookOpener;
  const bool dodh_route = dodh_trade == DodhTrade::PublicRoute && route;
  const bool dodh_longshadow = dodh_trade == DodhTrade::Longshadow;
  const unsigned kind = dodh ? Exchange::JournalKind : transform ? 3 : semantic ? 2 : 1;
  const bool kithicor = !dodh && !transform && !semantic;
  try {
   connection_owner=std::make_unique<ZoneDatabase>();auto& db=*connection_owner;
   const auto* cfg=EQEmuConfig::get();if(!cfg||!db.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"kithicor-poison-exclusive"))return true;
   uint64_t run = (kithicor || dodh_guard || dodh_longshadow) ? uint64_t(c.GetSharedTaskId()) : 0;
   if (dodh_route && route->group == 5 && c.GetSharedTaskId() > 0 && c.GetTaskState() &&
       c.GetTaskState()->GetActiveSharedTask().task_id == route->task &&
       c.IsTaskActivityActive(route->task, route->final_activity)) {
    run = static_cast<uint64_t>(c.GetSharedTaskId());
   }
   const auto accepted = dodh_route && route->group == 7 && c.IsTaskActive(route->task)
       ? std::max(0, c.GetTaskStartTime(TaskType::Task, 0)) : 0;
   const bool route_coin = dodh_route && route->group == 7 && route->code == 5;
   const bool route_food = dodh_route && route->group == 7 && route->code <= 4;
   const auto same_route = [&]() {
    return zone && Exchange::PublicRouteScene(*route, zone->GetZoneID(), zone->GetInstanceVersion(),
        zone->GetInstanceID(), npc.GetNPCTypeID(), npc.GetSpawnPointID());
   };
   const auto prior_route = [&]() {
    const auto predicate = route->group == 5
        ? fmt::format("shared_task_id={}", run)
        : fmt::format("shared_task_id=0 AND character_id={} AND account_id={}", c.CharacterID(), c.AccountID());
    const std::string location = Exchange::IsColdwindRoute(*route)
        ? fmt::format("instance_id IN (0,{})", Exchange::ColdwindGlobalInstance)
        : "instance_id=0";
    return Q(db, fmt::format(
        "SELECT request_key FROM kithicor_poison_custody WHERE {} AND recipe_kind=4 "
        "AND recipe_context=x'{}' AND {} AND phase=0 AND chest=0 AND physical=1 AND abort_only=0 FOR UPDATE",
        predicate, Hex(Exchange::RouteContext(*route, accepted)), location)).RowCount() != 0;
   };
   const auto lock_route = [&]() {
    if (!same_route()) throw std::runtime_error("Darkhollow public trade actor changed");
    if (route->group == 7 && (!accepted || c.GetTaskStartTime(TaskType::Task,0) != accepted ||
        !c.IsTaskActivityActive(route->task, route->final_activity)))
     throw std::runtime_error("Darkhollow functionary occurrence changed");
    if (route->group == 5) {
     const auto member = Q(db, fmt::format(
         "SELECT st.id FROM shared_tasks st JOIN shared_task_members sm ON sm.shared_task_id=st.id "
         "WHERE st.id={} AND st.task_id={} AND sm.character_id={} AND st.completion_time IS NULL "
         "AND (st.expire_time IS NULL OR st.expire_time>NOW()) FOR UPDATE",
         run, route->task, c.CharacterID()));
     if (!run || member.RowCount() != 1 || !c.IsTaskActivityActive(route->task, route->final_activity))
      throw std::runtime_error("Darkhollow public trade has no active owning shared task");
    }
    if (prior_route()) throw std::runtime_error("Darkhollow public trade is already recorded");
   };
   uint32_t guard_dz = 0;
   const auto* longshadow = dodh_longshadow ? MatchingLongshadowOffer(c, npc) : nullptr;
   std::optional<DodhMissionCustody::LockedDebit> longshadow_debit;
   if (dodh_longshadow && !longshadow) {
    c.Message(Chat::Yellow, "This mission cannot accept that hand-in yet.");
    return true;
   }
   const auto guard_context = [&]() -> std::string {
    const auto* live_task = c.GetTaskState();
    auto* dz = zone->GetDynamicZone();
    if (c.GetSharedTaskId() <= 0 || uint64_t(c.GetSharedTaskId()) != run || !live_task ||
        live_task->GetActiveSharedTask().task_id != Exchange::DesertingTask || !dz ||
        dz->GetType() != DynamicZoneType::Mission || !dz->GetID() ||
        dz->GetInstanceID() != zone->GetInstanceID() || dz->IsExpired() || !dz->HasMember(c.CharacterID()) ||
        !c.IsTaskActivityActive(Exchange::DesertingTask, 1) || npc.IsEngaged()) return {};
    if (zone->GetVariable("dodh_deserting_triangle_ready") != fmt::format("{}:{}", run, dz->GetID())) return {};
    const auto marker = npc.GetEntityVariable("rof2_dodh_scene_actor");
    if (marker.empty() || npc.GetEntityVariable("dodh_deserting_passive") != marker) return {};
    const auto prefix = fmt::format("{}:{}:", zone->GetInstanceID(), run);
    if (!Strings::BeginsWith(marker, prefix)) return {};
    const auto generation = SharedTaskAdmission::Number(marker.c_str() + prefix.size());
    if (!generation || !*generation || *generation > UINT32_MAX) return {};
    guard_dz = dz->GetID();
    return Exchange::EncodeGuard({run, guard_dz, npc.GetID(), static_cast<uint32_t>(*generation)});
   };
   const auto lock_guard_run = [&] {
    auto member = Q(db, fmt::format(
        "SELECT st.id FROM shared_tasks st JOIN shared_task_members sm ON sm.shared_task_id=st.id "
        "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=st.id "
        "JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id JOIN instance_list i ON i.id=dz.instance_id "
        "WHERE st.id={} AND st.task_id={} AND sm.character_id={} AND dz.id={} AND i.id={} "
        "AND i.zone={} AND i.version={} AND st.completion_time IS NULL "
        "AND (st.expire_time IS NULL OR st.expire_time>NOW()) FOR UPDATE",
        run, Exchange::DesertingTask, c.CharacterID(), guard_dz, zone->GetInstanceID(),
        Exchange::DesertingZone, Exchange::DesertingVersion));
    if (member.RowCount() != 1) throw std::runtime_error("Darkhollow guard hand-in has no current mission member");
   };
   if (kithicor && c.GetSharedTaskId() <= 0) return true;
   const auto canonical=[&] {
    if (dodh) {
     const bool claimed = dodh_guard ? DodhOriginalExchange::ClaimsGuardOffer(c, npc)
         : dodh_opener ? DodhOriginalExchange::ClaimsOpenerOffer(c, npc)
         : dodh_monocle ? DodhOriginalExchange::ClaimsMonocleOffer(c, npc)
         : dodh_route ? same_route() && DodhOriginalExchange::ClaimsPublicRoute(c,npc)
         : dodh_longshadow ? MatchingLongshadowOffer(c, npc) == longshadow
         : DodhOriginalExchange::ClaimsOffer(c, npc);
     if (c.IsSpiritShrouded() || !claimed || !c.trade || c.trade->With() != &npc ||
         !npc.GetID() || entity_list.GetNPCByID(npc.GetID()) != &npc || npc.GetDepop() || npc.GetHP() <= 0) {
      throw std::runtime_error("Darkhollow trade actor changed");
     }
     if (dodh_guard) {
      const auto observed = guard_context();
      if (observed.empty() || (!recipe_context.empty() && observed != recipe_context))
       throw std::runtime_error("Darkhollow guard actor generation or scene changed");
      recipe_context = observed;
     }
     return;
    }
    if(transform){if(chest||!zone||zone->GetZoneID()!=452||zone->GetInstanceVersion()||zone->GetInstanceID()||!c.trade||c.trade->With()!=&npc||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||(!resume&&!OriginalTradeTransforms::ClaimsOffer(c,npc)))throw std::runtime_error("Original transform actor changed");return;}
    if(!semantic){Canonical(db,c,npc,uint64_t(run),chest);return;}
    if(chest||!zone||zone->GetZoneID()!=452||zone->GetInstanceVersion()!=0||zone->GetInstanceID()!=0||!c.trade||c.trade->With()!=&npc||!npc.GetID()||entity_list.GetNPCByID(npc.GetID())!=&npc||npc.GetDepop()||!SodFoodDelivery::ClaimsOffer(c,npc))throw std::runtime_error("Food original trade changed");
   };
   canonical();
   if(c.trade->state!=Trading)return true;
   auto ticket=c.InventorySessionReleaseTicket();auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
   if(!lease||!c.HasInventorySessionReservation())return true;
   bool opener_repeat = false;
   if (dodh_opener) {
    if (!db.TransactionBeginStrict().Success()) return true;
    transaction = true;
    if (!Generalized(db)) throw std::runtime_error("Darkhollow opener journal unavailable");
    LockLease(db, *lease);
    recipe_context = Exchange::OpenerContext;
    auto prior = Q(db, fmt::format(
        "SELECT request_key FROM kithicor_poison_custody WHERE character_id={} "
        "AND recipe_kind={} AND recipe_context=x'{}' AND shared_task_id=0 AND phase=0 AND chest=0 "
        "AND instance_id=0 AND npc_type_id={} AND npc_spawn_id={} AND physical=1 AND settled=1 AND abort_only=0 FOR UPDATE",
        c.CharacterID(), kind, Hex(recipe_context), Exchange::OpenerNpc, Exchange::OpenerSpawn));
    if (prior.RowCount() > 1) throw std::runtime_error("Darkhollow opener receipt is ambiguous");
    opener_repeat = prior.RowCount() == 1;
   }
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
   std::unique_ptr<EQ::ItemInstance> exchange_output;
   if (dodh_exchange) {
    recipe_context = dodh_monocle ? Exchange::MonocleContext : Exchange::JournalContext;
    const auto output_item = dodh_monocle ? Exchange::Monocle : Exchange::Output;
    const bool planned = dodh_monocle ? Exchange::PlanParts(Exchange::MonocleParts, offers, debit)
                                      : Exchange::Plan(offers, debit);
    if (planned) {
     exchange_output.reset(database.CreateItem(output_item));
     if (!exchange_output || exchange_output->GetID() != output_item ||
         exchange_output->GetSerialNumber() <= 0 || c.CheckLoreConflict(exchange_output->GetItem())) {
      exchange_output.reset();
      debit = {};
     } else if (exchange_output->IsStackable()) {
      exchange_output->SetCharges(1);
     }
    }
   }
   bool route_accept = false;
   if (dodh_route) {
    recipe_context = Exchange::RouteContext(*route, accepted ? accepted : 1);
    bool planned = false;
    if (route->group == 7 && !accepted) {
     // Return relevant offers for a character without this active occurrence.
    } else if (route_food) {
     for (const auto& offer : offers) {
      const auto* item = offer.present ? database.GetItem(offer.item_id) : nullptr;
      if (item && item->ItemType == EQ::item::ItemTypeFood) {
       planned = Recipe::FirstUnit(offers, offer.item_id, debit);
       break;
      }
     }
    } else if (route_coin) {
     planned = c.trade->pp >= 240;
    } else {
     planned = Exchange::PlanRoute(*route, offers, debit);
    }
    if (planned && (route->group != 5 || run) &&
        (route->group != 7 || c.IsTaskActivityActive(route->task, route->final_activity))) {
     if (!db.TransactionBeginStrict().Success()) return true;
     transaction = true;
     LockLease(db, *lease);
     if (!prior_route()) route_accept = true;
     if (route_coin && route_accept) {
      exchange_output.reset(database.CreateItem(88009));
      if (!exchange_output || exchange_output->GetID() != 88009 || exchange_output->GetSerialNumber() <= 0 ||
          c.CheckLoreConflict(exchange_output->GetItem())) {
       exchange_output.reset();
       route_accept = false;
      }
     }
    }
    if (!route_accept) debit = {};
   }
   if (dodh_longshadow) {
    if (!db.TransactionBeginStrict().Success()) return true;
    transaction = true;
    LockLease(db, *lease);
    const bool valid_chest_multiplier = longshadow->effect != Longshadow::Effect::OpenBloodeyeChest ||
        RuleI(Zone, GlobalLootMultiplier) == 1;
    if (!valid_chest_multiplier || (Longshadow::SceneActor(longshadow->effect) &&
        !DodhMissionCustody::LockSceneFixture(db, *longshadow))) {
     c.Message(Chat::Yellow, "This encounter is unavailable. Your items remain in the trade window.");
     return true;
    }
    const auto context = BindLongshadow(db, c, npc, *longshadow);
    if (context) longshadow_debit = DodhMissionCustody::LockTurnin(db, *longshadow, *context, offers);
    if (!longshadow_debit) {
     c.Message(Chat::Yellow, "This mission could not verify the offered item. Your items remain in the trade window.");
     return true;
    }
    const auto& source = longshadow_debit->source;
    recipe_context = Longshadow::EncodeJournal({source.occurrence, source.accepted_time, source.task,
        longshadow->activity, source.mission_instance, uint32_t(source.guid)});
    debit = longshadow_debit->debit;
   }
   if (dodh_guard && !Recipe::FirstUnit(offers, Exchange::Guard, debit)) return true;
   if (dodh_opener && !opener_repeat && !Recipe::FirstUnit(offers, Exchange::Notebook, debit)) return true;
   if (kithicor && !Recipe::FirstUnit(offers,38504,debit)) return true;
   if(!Recipe::Valid(offers,debit))return true;
   EQ::ItemInstance* selected=nullptr;int16_t selected_slot=-1;
   for(size_t i=0;i<Recipe::OfferCount;++i)if(debit.units[i]){
    selected_slot=offers[i].slot;selected=inv.GetItem(selected_slot);break;
   }
   if(transform){selected_slot=transform_context->offers[transform_context->primary].slot;selected=inv.GetItem(selected_slot);}
   if ((dodh_exchange || opener_repeat || dodh_route) && !selected) {
    for (const auto& offer : offers) {
     const bool claimed_item = opener_repeat ? offer.item_id == Exchange::Notebook
         : dodh_monocle ? Exchange::IsMonoclePart(offer.item_id)
         : dodh_route ? (route_food && offer.present && database.GetItem(offer.item_id) &&
                           database.GetItem(offer.item_id)->ItemType == EQ::item::ItemTypeFood) ||
                          Exchange::RoutePart(*route, offer.item_id) : Exchange::IsPart(offer.item_id);
     if (offer.present && claimed_item) {
      selected_slot = offer.slot;
      selected = inv.GetItem(selected_slot);
      break;
     }
    }
   }
   if(!selected && !route_coin)return true;
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
   if (exchange_output) {
    output_guid = exchange_output->GetSerialNumber();
    staged.push(exchange_output.get());
    exchange_output.release();
   }
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
   refund(after_pp.copper,c.trade->cp);refund(after_pp.silver,c.trade->sp);refund(after_pp.gold,c.trade->gp);refund(after_pp.platinum,c.trade->pp - (route_coin && route_accept ? 240u : 0u));
   if(!resume)hold=std::unique_ptr<FungalInventory::UnknownInventory>(new FungalInventory::UnknownInventory);
   auto* active_hold=resume?resume->hold:hold.get();
   active_hold->character_=c.CharacterID();active_hold->inventory_=&inv;
   const auto remember=[&](const auto& self,const EQ::ItemInstance* p)->void {if(!p)return;active_hold->items_.push_back(p);for(const auto& child:*p->GetContents())self(self,child.second);};
   for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& entry:*bucket)remember(remember,entry.second);
   for(auto* p:before_queue)remember(remember,p);for(auto* p:staged)remember(remember,p);
   uint8_t nonce[16];if(!resume&&RAND_bytes(nonce,sizeof(nonce))!=1)return true;auto key=resume?resume->key:Hex(nonce,sizeof(nonce));if(transform&&!resume)for(auto& ch:key)if(ch>='A'&&ch<='F')ch=char(ch-'A'+'a');
   auto finish=std::make_unique<EQApplicationPacket>(OP_FinishTrade,0);
   Ack ack{&c,&npc,selected,uint64_t(run),zone->GetInstanceID(),chest,key,static_cast<int16_t>(selected ? selected_slot : 0), selected ? selected->GetSerialNumber() : 0, static_cast<int16_t>(selected ? selected->GetCharges() : 0),false};
   OriginalTradeWriter::Intent food_intent;
   if(semantic){food_intent.request_key=key;food_intent.snapshot=*food_snapshot;}
   std::vector<EQ::ItemInstance*> delivered{selected};
   std::unique_ptr<EQApplicationPacket> cursor_packet;
   if(!staged.empty()){const auto bytes=staged.peek_front()->Serialize(EQ::invslot::slotCursor);cursor_packet=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));auto* wire=reinterpret_cast<ItemPacket_Struct*>(cursor_packet->pBuffer);wire->PacketType=ItemPacketTrade;std::memcpy(wire->SerializedItem,bytes.data(),bytes.size());}
   auto connection_row=Q(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
   if(connection_row.RowCount()!=1||N(connection_row.begin()[1])!=(transaction?1:0)||N(connection_row.begin()[2])!=1)return true;
   const auto connection=N(connection_row.begin()[0]);
   if(!transaction){if(!db.TransactionBeginStrict().Success())return true;transaction=true;}
   if(!Installed(db))throw std::runtime_error("Kithicor custody schema absent");
   if (dodh) {
    auto engines = Q(db, "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases','kithicor_poison_custody')");
    if (engines.RowCount() != 1 || N(engines.begin()[0]) != 5) throw std::runtime_error("Darkhollow exchange transaction unavailable");
   } else {
    Foundation(db,semantic!=nullptr,transform);
   }
   LockLease(db,*lease);
   auto housing=Q(db,"SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(c.CharacterID())+" FOR UPDATE");
   if(housing.RowCount()>1||(housing.RowCount()&&(N(housing.begin()[0])!=c.AccountID()||(housing.begin()[1]&&*housing.begin()[1]))))throw std::runtime_error("Kithicor other inventory custody pending");
   if(!resume&&KithicorPoisonCustody::Pending(db,c.CharacterID()))throw std::runtime_error("Kithicor old custody pending");
   canonical();ValidateTrees(db,c,before_queue,true);ValidateCurrency(db,c,c.GetPP());
   if(!Recipe::Same(offers,OriginalTradeOffer::Capture(c))||!Recipe::Valid(offers,debit))throw std::runtime_error("Kithicor original offer plan changed");
   if (dodh_guard) {
    lock_guard_run();
    auto prior = Q(db, fmt::format(
        "SELECT request_key FROM kithicor_poison_custody WHERE shared_task_id={} AND phase=0 AND chest=0 "
        "AND recipe_kind={} AND LEFT(recipe_context,6)=x'{}' AND (physical=1 OR settled=0) FOR UPDATE",
        run, kind, Hex(std::string(Exchange::GuardPrefix))));
    if (prior.RowCount()) throw std::runtime_error("Darkhollow guard hand-in already accepted or pending");
   }
   if (route_accept) lock_route();
   if(kithicor){auto occupied=Q(db,fmt::format("SELECT status FROM kithicor_mission_actions WHERE shared_task_id={} AND phase=0 AND action_id={} FOR UPDATE",run,chest));
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
   if((semantic||dodh)&&!generalized)throw std::runtime_error("Original trade generalized journal absent");
   const auto insert_sql=[&](unsigned physical) {
    auto columns=std::string("request_key,character_id,account_id,inventory_epoch,inventory_token,process_key,client_key,instance_id,shared_task_id,phase,chest,npc_type_id,npc_entity_id,npc_spawn_id,source_slot,source_guid,source_charges,preimage,return_plan,postimage,physical,settled,settled_at,created_at");
    auto values=fmt::format("'{}',{},{},{},x'{}',x'{}',x'{}',{},{},0,{},{},{},{},{},{},{},x'{}',x'{}',NULL,{},0,NULL,UNIX_TIMESTAMP()",key,c.CharacterID(),c.AccountID(),lease->epoch,Hex(lease->token.data(),lease->token.size()),Hex(lease->owner.process_key),Hex(lease->owner.client_key),zone->GetInstanceID(),run,chest,npc.GetNPCTypeID(),npc.GetID(),npc.GetSpawnPointID(),selected ? selected_slot : 0,selected ? selected->GetSerialNumber() : 0,selected && selected->GetCharges()>=0 ? selected->GetCharges() : 32767,Hex(pre),Hex(abort_bytes),physical);
    if(generalized){columns+=",recipe_kind,recipe_context,abort_only";values+=","+std::to_string(kind)+","+(semantic||transform||dodh?"x'"+Hex(recipe_context)+"'":"NULL")+","+std::to_string(physical==3?1:0);}
    return "INSERT INTO kithicor_poison_custody("+columns+") VALUES("+values+")";
   };
   const auto intent_insert=insert_sql(0);abort_insert=insert_sql(3);
   if(prepared_continuation){prepared_continuation->key=key;prepared_continuation->pre=pre;prepared_continuation->abort_plan=abort_bytes;prepared_continuation->context=*transform_context;prepared_continuation->lease=*lease;prepared_continuation->inventory=&inv;}
   if(!resume){
   retained_lease=*lease;retained_key=key;retained_pre=pre;retained_plan=abort_bytes;
   persistence_attempted=true;
   if(kithicor)Q(db,fmt::format("INSERT INTO kithicor_mission_actions(shared_task_id,phase,action_id,character_id,item_id,status,updated_at) VALUES({},0,{},{},38504,0,UNIX_TIMESTAMP())",run,chest,c.CharacterID()));
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
   if (dodh_guard) lock_guard_run();
   if (route_accept) lock_route();
   if (longshadow_debit) {
    const auto context = BindLongshadow(db, c, npc, *longshadow);
    const auto locked = context ? DodhMissionCustody::LockTurnin(db, *longshadow, *context, offers)
                                : std::nullopt;
    if (!locked || locked->original_guid != longshadow_debit->original_guid ||
        locked->source.guid != longshadow_debit->source.guid ||
        locked->source.occurrence != longshadow_debit->source.occurrence ||
        locked->source.accepted_time != longshadow_debit->source.accepted_time ||
        locked->source.mission_instance != longshadow_debit->source.mission_instance ||
        locked->debit.units != debit.units)
     throw std::runtime_error("Longshadow original offer provenance changed");
    longshadow_debit = locked;
   }
   auto intent=Q(db,"SELECT request_key FROM kithicor_poison_custody WHERE request_key='"+key+"' AND physical=0 AND settled=0 AND "+LeaseWhere(*lease)+" FOR UPDATE");
   if(intent.RowCount()!=1||Snapshot(db,c.CharacterID())!=pre||!ticket->HeldFor(c))throw std::runtime_error("Kithicor intent physical preimage changed");
   if(!Recipe::Same(offers,OriginalTradeOffer::Capture(c)))throw std::runtime_error("Original offer changed before physical debit");
   if(semantic||transform||dodh){
    auto context=Q(db,"SELECT recipe_kind,HEX(recipe_context),abort_only FROM kithicor_poison_custody WHERE request_key='"+key+"' AND "+LeaseWhere(*lease)+" FOR UPDATE");
    if(context.RowCount()!=1||N(context.begin()[0])!=kind||!context.begin()[1]||std::string(context.begin()[1])!=Hex(recipe_context)||N(context.begin()[2]))throw std::runtime_error("Original trade immutable physical intent differs");
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
   if (dodh_monocle && output_guid) {
    // The old Hive key must survive consuming the device. Persist it with the
    // same exclusive transaction and character lease as the physical exchange.
    auto engine = Q(db, "SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='keyring'");
    if (engine.RowCount() != 1 || !engine.begin()[0] || std::string_view(engine.begin()[0]) != "InnoDB")
     throw std::runtime_error("Darkhollow public key table is not transactional");
    const auto keyring = Q(db, fmt::format("SELECT id FROM keyring WHERE char_id={} AND item_id={} FOR UPDATE",
                                         c.CharacterID(), Exchange::Output));
    if (!keyring.RowCount() &&
        Q(db, fmt::format("INSERT INTO keyring (char_id,item_id) VALUES ({},{})",
                          c.CharacterID(), Exchange::Output)).RowsAffected() != 1)
     throw std::runtime_error("Darkhollow public key preservation failed");
   }
   if (longshadow_debit && !DodhMissionCustody::RecordSettlement(db, *longshadow, *longshadow_debit, key))
    throw std::runtime_error("Longshadow physical settlement receipt refused");
   const unsigned physical_outcome = ((dodh_exchange && !output_guid) || opener_repeat || (dodh_route && !route_accept)) ? 2 : 1;
   auto outcome=Q(db,"UPDATE kithicor_poison_custody SET physical="+std::to_string(physical_outcome)+",postimage=x'"+Hex(post)+"' WHERE request_key='"+key+"' AND physical=0 AND settled=0 AND "+LeaseWhere(*lease));
   if(outcome.RowsAffected()!=1||db.TransactionStrictFailed())throw std::runtime_error("Kithicor physical outcome changed");
   Session(db,connection);if(!db.TransactionCommitStrict().Success())throw std::runtime_error("Kithicor physical commit unknown");transaction=false;
   // Allocation-free native ownership publication. staged now owns old cursor.
   inv.m_cursor.Swap(staged);std::array<std::unique_ptr<EQ::ItemInstance>,4> spent;
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s){auto at=inv.m_trade.find(s);if(at!=inv.m_trade.end()){spent[s-EQ::invslot::TRADE_BEGIN].reset(at->second);at->second=nullptr;}}
   c.GetPP()=after_pp;
   if (dodh_monocle && output_guid && !c.KeyRingCheck(Exchange::Output)) {
    c.KeyRingLoad();
    if (!c.KeyRingCheck(Exchange::Output))
     throw std::runtime_error("Darkhollow public key publication requires reload");
   }
   auto settled=Q(db,"UPDATE kithicor_poison_custody SET settled=1,settled_at=UNIX_TIMESTAMP() WHERE request_key='"+key+"' AND physical="+std::to_string(physical_outcome)+" AND settled=0 AND "+LeaseWhere(*lease));
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
   } else if (kithicor) {
    Canonical(db,c,npc,uint64_t(run),chest);
    struct Scope {Ack* old;Scope(Ack& a):old(current_ack){current_ack=&a;}~Scope(){current_ack=old;}} scope(ack);
    const bool updated=c.UpdateTasksOnDeliver(delivered,*c.trade,&npc);
    bool gameplay_accepted=false;
    try{auto receipt=Q(db,fmt::format("SELECT status FROM kithicor_mission_actions WHERE shared_task_id={} AND phase=0 AND action_id={} AND character_id={} AND item_id=38504 AND status=1",run,chest,c.CharacterID()));gameplay_accepted=updated&&receipt.RowCount()==1;}catch(...){}
    if(!gameplay_accepted)c.Message(Chat::Yellow,"Your vial was used, but task progress could not be confirmed.");
   }
   if (route_accept) {
    if (route->group == 7 && c.IsTaskActivityActive(route->task, route->final_activity)) {
     c.UpdateTaskActivity(route->task, route->final_activity, 1);
    }
    if (route->group == 5 && c.GetSharedTaskId() > 0 && static_cast<uint64_t>(c.GetSharedTaskId()) == run &&
        c.IsTaskActivityActive(route->task, route->final_activity)) {
     c.UpdateTaskActivity(route->task, route->final_activity, 1);
    }
    c.Message(Chat::Yellow, "Your offering has been accepted.");
   } else if (dodh_route) {
    c.Message(Chat::Yellow, "Your offering was returned.");
   }
   if (opener_repeat) {
    c.Message(Chat::Yellow, "You have already turned in a notebook. Your offer was returned.");
   } else if (dodh_exchange && !output_guid) {
    c.Message(Chat::Yellow, "The exchange could not be completed. Your offer was returned.");
   }
   c.trade->Reset();c.QueuePacket(finish.get());if(cursor_packet)c.QueuePacket(cursor_packet.get());c.SendMoneyUpdate();c.RecalcWeight();
   if (dodh_route && route->group == 5 && route->code == 1 && !route_accept && c.GetSharedTaskId() <= 0 &&
       std::any_of(offers.begin(), offers.end(), [](const auto& item) { return item.present && item.item_id == 88006; }) &&
       std::none_of(offers.begin(), offers.end(), [](const auto& item) { return item.present && item.item_id == 88007; })) {
    c.TaskQuestSetSelector(&npc, {static_cast<int>(route->task)}, false);
   }
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

namespace DodhRakbanCombine {
std::optional<Offer> Capture(Client& client, int16_t slot, const EQ::ItemInstance& bag)
{
 if (slot < EQ::invslot::GENERAL_BEGIN || slot > EQ::invslot::GENERAL_END ||
     !bag.IsClassBag() || bag.GetID() != Shroud || bag.GetSerialNumber() <= 0 ||
     bag.GetContents()->size() != 5) return std::nullopt;
 Offer result{client.CharacterID(), client.AccountID(), slot, bag.GetSerialNumber()};
 for (const auto& [cell, fragment] : *bag.GetContents()) {
  if (!fragment || cell >= bag.GetItem()->BagSlots || cell >= EQ::invbag::SLOT_COUNT ||
      fragment->GetID() < 85566 || fragment->GetID() > 85570 ||
      fragment->GetSerialNumber() <= 0 || fragment->GetSerialNumber() == result.shroud ||
      fragment->IsClassBag() || fragment->IsEvolving() || !fragment->GetContents()->empty() ||
      (fragment->IsStackable() && fragment->GetCharges() != 1)) return std::nullopt;
  const auto augments = fragment->GetAugmentIDs();
  if (std::any_of(augments.begin(), augments.end(), [](uint32_t id) { return id != 0; }))
   return std::nullopt;
  const auto i = fragment->GetID() - 85566;
  if (result.fragments[i] || std::find(result.fragments.begin(), result.fragments.end(),
      fragment->GetSerialNumber()) != result.fragments.end()) return std::nullopt;
  result.cells[i] = cell;
  result.fragments[i] = fragment->GetSerialNumber();
 }
 return result;
}

void Manual(Client& client, const Offer& offer, const DBTradeskillRecipe_Struct& recipe)
{
 KithicorPoisonTrade::Writer::CombineRakban(client, offer, recipe);
}
}

namespace OriginalMessageAdmission {
std::optional<PendingSelection> FindForNativeSelection(Client& c,uint32_t task,uint32_t giver){return KithicorPoisonTrade::Writer::FindAdmission(c,task,giver);}
}
namespace OriginalTradeTransforms {
void DeclineOriginalMessageTrade(Client& c)noexcept{KithicorPoisonTrade::Writer::DeclineMessageTrade(c);}
}

namespace DodhOriginalExchange {
bool ClaimsOffer(Client& client, NPC& npc)
{
    namespace Exchange = EQEmu::SharedCore::DodhExchange;
    if (!zone || zone->GetZoneID() != Exchange::Zone ||
        zone->GetInstanceVersion() || zone->GetInstanceID() ||
        npc.GetNPCTypeID() != Exchange::Giver || npc.GetSpawnPointID() != Exchange::Spawn) {
        return false;
    }
    const auto offers = OriginalTradeOffer::Capture(client);
    return std::any_of(offers.begin(), offers.end(), [](const auto& offer) {
        return offer.present && Exchange::IsPart(offer.item_id);
    });
}

const EQEmu::SharedCore::DodhExchange::PublicRoute* OfferedPublicRoute(Client& client, NPC& npc)
{
    namespace Exchange = EQEmu::SharedCore::DodhExchange;
    if (!zone) return nullptr;
    const auto offers = OriginalTradeOffer::Capture(client);
    const auto* task_state = client.GetTaskState();
    for (const auto& route : Exchange::PublicRoutes) {
        if (!Exchange::RouteReady(route) ||
            !Exchange::PublicRouteScene(route, zone->GetZoneID(), zone->GetInstanceVersion(),
                zone->GetInstanceID(), npc.GetNPCTypeID(), npc.GetSpawnPointID())) continue;
        if (route.group == 5 && (!task_state || client.GetSharedTaskId() <= 0 ||
            task_state->GetActiveSharedTask().task_id != route.task ||
            !client.IsTaskActivityActive(route.task, route.final_activity))) continue;
        if (route.group == 7 && (route.task != 505745 || !client.IsTaskActive(route.task) ||
            !client.IsTaskActivityActive(route.task, route.final_activity) ||
            client.GetTaskStartTime(TaskType::Task, 0) <= 0)) continue;
        const bool coin = route.group == 7 && route.code == 5 && client.trade && client.trade->pp >= 240;
        const bool part = std::any_of(offers.begin(), offers.end(), [&](const auto& offer) {
            if (!offer.present) return false;
            if (Exchange::RoutePart(route, offer.item_id)) return true;
            if (route.group != 7 || route.code > 4) return false;
            const auto* item = database.GetItem(offer.item_id);
            return item && item->ItemType == EQ::item::ItemTypeFood;
        });
        if (part || coin) return &route;
    }
    return nullptr;
}

bool ClaimsPublicRoute(Client& client, NPC& npc)
{
    return OfferedPublicRoute(client,npc) != nullptr;
}

bool ClaimsMonocleOffer(Client& client, NPC& npc)
{
    namespace Exchange = EQEmu::SharedCore::DodhExchange;
    if (!zone || zone->GetZoneID() != Exchange::Zone || zone->GetInstanceVersion() ||
        zone->GetInstanceID() || npc.GetNPCTypeID() != Exchange::Giver ||
        npc.GetSpawnPointID() != Exchange::Spawn) {
        return false;
    }
    const auto offers = OriginalTradeOffer::Capture(client);
    return std::any_of(offers.begin(), offers.end(), [](const auto& offer) {
        return offer.present && Exchange::IsMonoclePart(offer.item_id);
    });
}

bool ClaimsGuardOffer(Client& client, NPC& npc)
{
    namespace Exchange = EQEmu::SharedCore::DodhExchange;
    if (!zone || zone->GetZoneID() != Exchange::DesertingZone ||
        zone->GetInstanceVersion() != Exchange::DesertingVersion || !zone->GetInstanceID() ||
        !Exchange::GuardActor(npc.GetNPCTypeID(), npc.GetSpawnPointID())) return false;
    const auto offers = OriginalTradeOffer::Capture(client);
    return std::any_of(offers.begin(), offers.end(), [](const auto& offer) {
        return offer.present && offer.item_id == Exchange::Guard;
    });
}

bool ClaimsOpenerOffer(Client& client, NPC& npc)
{
    namespace Exchange = EQEmu::SharedCore::DodhExchange;
    if (!zone || zone->GetZoneID() != Exchange::Zone || zone->GetInstanceVersion() ||
        zone->GetInstanceID() || npc.GetNPCTypeID() != Exchange::OpenerNpc ||
        npc.GetSpawnPointID() != Exchange::OpenerSpawn) return false;
    const auto offers = OriginalTradeOffer::Capture(client);
    return std::any_of(offers.begin(), offers.end(), [](const auto& offer) {
        return offer.present && offer.item_id == Exchange::Notebook;
    });
}

bool ClaimsLongshadowOffer(Client& client, NPC& npc)
{
    return KithicorPoisonTrade::MatchingLongshadowOffer(client, npc) != nullptr;
}

bool OriginalAccept(Client& client, NPC& npc)
{
    using Mode = KithicorPoisonTrade::Writer::DodhTrade;
    if (ClaimsLongshadowOffer(client, npc)) {
        return KithicorPoisonTrade::Writer::Execute(client, npc, 0, nullptr, false, nullptr, Mode::Longshadow);
    }
    if (ClaimsOpenerOffer(client, npc)) {
        return KithicorPoisonTrade::Writer::Execute(client, npc, 0, nullptr, false, nullptr, Mode::NotebookOpener);
    }
    if (ClaimsGuardOffer(client, npc)) {
        return KithicorPoisonTrade::Writer::Execute(client, npc, 0, nullptr, false, nullptr, Mode::DesertingGuard);
    }
    if (const auto* route = OfferedPublicRoute(client,npc)) {
        return KithicorPoisonTrade::Writer::Execute(client, npc, 0, nullptr, false, nullptr, Mode::PublicRoute, route);
    }
    if (ClaimsMonocleOffer(client, npc)) {
        return KithicorPoisonTrade::Writer::Execute(client, npc, 0, nullptr, false, nullptr, Mode::Monocle);
    }
    return ClaimsOffer(client, npc) &&
        KithicorPoisonTrade::Writer::Execute(client, npc, 0, nullptr, false, nullptr, Mode::Exchange);
}
}

namespace DodhLongshadowSource {
namespace P = EQEmu::SharedCore::DodhLongshadowCustody;
constexpr const char* CorpseProof = "rof2_longshadow_corpse_source";
constexpr const char* SceneRunProof = "rof2_dodh_longshadow_scene_run";

bool Matches(const P::SourceRoute& route, const NPC& npc)
{
    return zone && zone->GetZoneID() == route.zone && zone->GetInstanceVersion() == route.version &&
        npc.GetNPCTypeID() == route.npc && (!route.spawn || npc.GetSpawnPointID() == route.spawn);
}

void RetryPendingNote(Client& client)
{
    const auto& route = P::Sources[0];
    if (!zone || zone->GetZoneID() != route.zone || zone->GetInstanceVersion() != route.version ||
        zone->GetInstanceID() || client.GetSharedTaskId() <= 0 ||
        !client.IsTaskActive(route.task)) return;
    auto due = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM dodh_longshadow_note_events note "
        "JOIN shared_tasks live ON live.id=note.shared_task_id AND live.task_id=920251 "
        "AND UNIX_TIMESTAMP(live.accepted_time)=note.accepted_time "
        "WHERE live.id={} AND note.character_id={} AND live.completion_time IS NULL "
        "AND NOT EXISTS (SELECT 1 FROM dodh_longshadow_item_sources source "
        "WHERE source.shared_task_id=live.id AND source.source_kind='dustspirit_note')",
        client.GetSharedTaskId(), client.CharacterID()), false);
    if (due.Success() && due.RowCount() == 1)
        KithicorPoisonTrade::Writer::ClaimDodhCursor(client, std::nullopt, &route);
}

void Accepted(Client& client, NPC& npc, uint32_t task, uint32_t accepted_time)
{
    const auto& route = P::Sources[0];
    if (task != route.task || !accepted_time || !Matches(route, npc) || zone->GetInstanceID()) return;
    // World sends the real NPC context only to the requesting character,
    // before its separate native dynamic-zone creation finishes.
    auto reserved = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_longshadow_note_events (shared_task_id,character_id,accepted_time) "
        "SELECT live.id,m.character_id,UNIX_TIMESTAMP(live.accepted_time) FROM shared_tasks live "
        "JOIN shared_task_members m ON m.shared_task_id=live.id AND m.character_id={} "
        "WHERE live.task_id={} AND UNIX_TIMESTAMP(live.accepted_time)={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "ON DUPLICATE KEY UPDATE shared_task_id=dodh_longshadow_note_events.shared_task_id",
        client.CharacterID(), task, accepted_time), false);
    if (reserved.Success()) RetryPendingNote(client);
}

bool OnTargetedSay(Client& client, NPC& npc, const char* message)
{
    if (!message || !zone || npc.GetDepop() || npc.GetHP() <= 0 || client.GetTarget() != &npc) return false;
    const auto lower = Strings::ToLower(message);
    if (!Strings::BeginsWith(lower, "hail") ||
        (lower.size() > 4 && lower[4] != ' ' && lower[4] != ',' && lower[4] != '!')) return false;
    if (DodhLongshadowTravel::TakeWhiteclawTravel(client, npc)) return true;
    if (DodhLongshadowTravel::ConfrontTraitor(client, npc)) return true;
    if (DodhBellfastNeran::Hail(client, npc)) return true;
    const auto* clicktok = P::FindSourceActivity(920270, 2);
    if (clicktok && zone->GetInstanceID() && Matches(*clicktok, npc) &&
        client.IsTaskActivityActive(clicktok->task, clicktok->activity)) {
        // Both legitimate routes settle the same unique per-occurrence gem source.
        KithicorPoisonTrade::Writer::ClaimDodhCursor(
            client, std::nullopt, clicktok, nullptr, 0, nullptr, 0, true);
        return true;
    }
    const auto& route = P::Sources[3];
    if (!Matches(route, npc) || zone->GetInstanceID()) return false;
    const bool returning = client.IsTaskActivityActive(route.task, 4);
    if (!returning && !client.IsTaskActivityActive(route.task, route.activity)) return false;
    if (returning) {
        DodhLongshadowTravel::Record(client, *DodhLongshadowTravel::Find(route.task), 4);
    } else {
        KithicorPoisonTrade::Writer::ClaimDodhCursor(client, std::nullopt, &route);
    }
    return true;
}

void MarkNativeCorpse(NPC& npc, Corpse& corpse)
{
    if (!zone || !zone->GetInstanceID()) return;
    if (DodhHardNotebookChest::IsActor(npc)) {
        corpse.SetEntityVariable(DodhHardNotebookChest::Marker, npc.GetEntityVariable(DodhHardNotebookChest::Marker));
        return;
    }
    DodhLongshadowScene::StampCorpse(npc, corpse);
    for (const auto& route : P::Sources) {
        if (route.origin != P::Origin::Corpse || !Matches(route, npc)) continue;
        // Dynamic actors and the randomized chest require their author's
        // exact run stamp. The retained static Poisonmaster has a fixed spawn.
        const auto stamp = route.source == P::Source::PoisonmasterCorpse
            ? npc.GetEntityVariable(DodhLongshadowTravel::OrbTerminal) : npc.GetEntityVariable(SceneRunProof);
        if ((!route.spawn || route.source == P::Source::PoisonmasterCorpse) && stamp.empty()) return;
        corpse.SetEntityVariable(CorpseProof, fmt::format("{}:{}", route.task, stamp));
        return;
    }
}

LootResult NativeBellfastGroundPickup(Client& client, const EQ::ItemInstance& item)
{
    const auto* route = P::FindSourceActivity(920270, 1);
    if (!route || route->origin != P::Origin::Ground || !zone || !zone->GetInstanceID() ||
        zone->GetZoneID() != route->zone || zone->GetInstanceVersion() != route->version ||
        !client.IsTaskActivityActive(route->task, route->activity) || item.GetID() != route->item)
        return LootResult::Refused;
    using Cursor = KithicorPoisonTrade::Writer::CursorResult;
    switch (KithicorPoisonTrade::Writer::ClaimDodhCursor(client, std::nullopt, route, &item)) {
    case Cursor::Issued: return LootResult::Issued;
    case Cursor::AlreadyIssued: return LootResult::AlreadyIssued;
    case Cursor::InventoryReload: return LootResult::InventoryReload;
    default: return LootResult::Refused;
    }
}

LootResult NativeLoot(Client& client, Corpse& corpse, const EQ::ItemInstance& item)
{
    if (!zone || !zone->GetInstanceID() || !corpse.IsNPCCorpse()) return LootResult::Unrelated;
    const auto actor = corpse.GetNPCTypeID();
    const auto* scene_route = DodhLongshadowScene::RouteForActor(actor);
    const auto scene = scene_route && DodhLongshadowScene::InScope(*scene_route)
        ? DodhLongshadowScene::ReadIdentity(corpse) : std::nullopt;
    const auto result = [](auto cursor) {
        using Cursor = KithicorPoisonTrade::Writer::CursorResult;
        switch (cursor) {
        case Cursor::Issued: return LootResult::Issued;
        case Cursor::AlreadyIssued: return LootResult::AlreadyIssued;
        case Cursor::InventoryReload: return LootResult::InventoryReload;
        default: return LootResult::Refused;
        }
    };
    if (DodhHardNotebookChest::IsCorpse(corpse)) {
        const auto run = DodhHardNotebookChest::MemberRun(client);
        if (!run || corpse.GetEntityVariable(DodhHardNotebookChest::Marker) != std::to_string(run))
            return LootResult::Refused;
        return result(KithicorPoisonTrade::Writer::ClaimDodhCursor(
            client, std::nullopt, nullptr, &item, 0, nullptr, run));
    }
    if (scene) {
        if (!P::SceneLoot(actor, item.GetID()) || !DodhLongshadowScene::AllowLoot(client, corpse))
            return LootResult::Refused;
        if (actor == 363035)
            return result(KithicorPoisonTrade::Writer::ClaimDodhCursor(
                client, std::nullopt, nullptr, &item, 0, &*scene));
    }
    for (const auto& route : P::Sources) {
        if (route.origin != P::Origin::Corpse || route.item != item.GetID() ||
            route.npc != corpse.GetNPCTypeID() || route.zone != zone->GetZoneID() ||
            route.version != zone->GetInstanceVersion()) continue;
        if (client.GetSharedTaskId() <= 0 || !client.IsTaskActivityActive(route.task, route.activity) ||
            corpse.GetEntityVariable(CorpseProof) !=
                (route.spawn && route.source != P::Source::PoisonmasterCorpse ? fmt::format("{}:", route.task)
                  : fmt::format("{}:{}", route.task, client.GetSharedTaskId())))
            return LootResult::Refused;
        return result(KithicorPoisonTrade::Writer::ClaimDodhCursor(
            client, std::nullopt, &route, &item, 0, scene ? &*scene : nullptr));
    }
    return LootResult::Unrelated;
}
}

namespace DodhSpellHail {
namespace {
void ReconcileRakbanMaster(Client& client)
{
    if (!client.IsTaskActive(DodhRakbanClaim::kMasterTask) || client.IsSpiritShrouded()) return;
    auto result = database.QueryDatabase(fmt::format(
        "SELECT route_id,item_id FROM dodh_rakban_claims "
        "WHERE character_id={} AND spell_id=0 AND item_serial>0 ORDER BY route_id",
        client.CharacterID()));
    if (!result.Success()) return;
    for (auto row : result) {
        const auto slot = SharedTaskAdmission::Number(row[0]).value_or(0);
        const auto item = SharedTaskAdmission::Number(row[1]).value_or(0);
        if (!slot || slot > DodhRakbanClaim::kShroudClaim ||
            item != (slot == DodhRakbanClaim::kShroudClaim ? 85575 : 85565 + slot)) continue;
        const auto activity = static_cast<int>(slot - 1);
        if (client.IsTaskActivityActive(DodhRakbanClaim::kMasterTask, activity)) {
            client.UpdateTaskActivity(DodhRakbanClaim::kMasterTask, activity, 1);
        }
    }
}
}

bool OnTargetedSay(Client& client, NPC& npc, const char* message) noexcept
{
    if (!message || !zone || zone->GetZoneID() != 362 || zone->GetInstanceVersion() ||
        zone->GetInstanceID() || npc.GetDepop() || npc.GetHP() <= 0 ||
        client.GetTarget() != &npc) return false;
    const bool spell_brother = npc.GetNPCTypeID() == 362107 && npc.GetSpawnPointID() == 102605;
    const bool fortune_teller = DodhRakbanClaim::kMasterEnabled && !client.IsSpiritShrouded() &&
        npc.GetNPCTypeID() == DodhRakbanClaim::kFortuneTeller &&
        npc.GetSpawnPointID() == DodhRakbanClaim::kFortuneTellerSpawn;
    if (!spell_brother && !fortune_teller) return false;
    try {
        const auto lower = Strings::ToLower(message);
        if (fortune_teller && lower == "begin the reconstruction") {
            if (!client.IsTaskActive(DodhRakbanClaim::kMasterTask) &&
                !client.IsTaskCompleted(DodhRakbanClaim::kMasterTask)) {
                client.AssignTask(DodhRakbanClaim::kMasterTask, npc.GetID(), true);
            }
            return false;
        }
        if (!Strings::BeginsWith(lower, "hail") ||
            (lower.size() > 4 && lower[4] != ' ' && lower[4] != ',' && lower[4] != '!')) return false;
        if (spell_brother) {
            for (const auto route : DodhSpellReward::kHailRoutes) {
                if (KithicorPoisonTrade::Writer::ClaimDodhHailItem(client, route)) return true;
            }
        } else {
            ReconcileRakbanMaster(client);
            if (!client.IsTaskActive(DodhRakbanClaim::kMasterTask)) {
                if (!client.IsTaskCompleted(DodhRakbanClaim::kMasterTask)) {
                    client.Message(Chat::White, "Say 'begin the reconstruction' to accept the quest.");
                }
                return false;
            }
            for (unsigned slot = 1; slot <= DodhRakbanClaim::kShroudClaim; ++slot) {
                bool delivered = false;
                if (KithicorPoisonTrade::Writer::ClaimDodhHailItem(client, std::nullopt, slot, &delivered)) {
                    return true;
                }
                if (delivered) {
                    ReconcileRakbanMaster(client);
                    break;
                }
            }
        }
        return false;
    } catch (...) {
        return false;
    }
}
}
