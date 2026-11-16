// Included once by trading.cpp. Original accept is the sole writer entrance.
#include "inventory_session_owner.h"
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
void Foundation(Database& db) {
 auto r=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_currency','housing_inventory_sessions','housing_live_item_leases','kithicor_poison_custody','kithicor_mission_actions','kithicor_mission_phases','shared_tasks','shared_task_members','shared_task_dynamic_zones','dynamic_zones','dynamic_zone_members','instance_list')");
 if(r.RowCount()!=1||N(r.begin()[0])!=13)throw std::runtime_error("Kithicor transactional foundation differs");
}
void Session(Database& db,uint64_t connection) {
 auto r=Q(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
 if(r.RowCount()!=1||N(r.begin()[0])!=connection||N(r.begin()[1])!=1||N(r.begin()[2])!=1||db.TransactionStrictFailed())throw std::runtime_error("Kithicor strict connection changed");
}
// An uncertain FIRST intent commit must not destroy the only live return plan.
// Retain the actual Client/session and resolve on fresh exclusive connections.
// If the original transaction did not commit, establish abort-only physical
// custody without claiming an action reservation or ever replaying the debit.
void EnsureAbortIntent(const Lease& lease,const std::string& key,const std::string& pre,const std::string& plan,const std::string& abort_insert) {
 for(;;){try{
  const auto* cfg=EQEmuConfig::get();if(!cfg)throw std::runtime_error("Kithicor configuration absent");
  Database fresh;if(!fresh.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"kithicor-intent-resolution"))throw std::runtime_error("Kithicor fresh intent connection unavailable");
  if(!fresh.TransactionBeginStrict().Success())throw std::runtime_error("Kithicor intent resolution unavailable");
  if(!Installed(fresh))throw std::runtime_error("Kithicor intent schema unavailable");LockLease(fresh,lease);
  // Locking the actual lease on this fresh connection also waits out the old
  // exclusive writer. Neither row absence nor best-effort rollback is proof.
  if(Snapshot(fresh,lease.actor.character_id)!=pre)throw std::runtime_error("Kithicor original intent preimage unresolved");
  auto found=Q(fresh,"SELECT physical,settled,HEX(preimage),HEX(return_plan) FROM kithicor_poison_custody WHERE request_key='"+key+"' AND "+LeaseWhere(lease)+" FOR UPDATE");
  if(found.RowCount()){
   if(found.RowCount()!=1||(N(found.begin()[0])!=0&&N(found.begin()[0])!=3)||N(found.begin()[1])||!found.begin()[2]||std::string(found.begin()[2])!=Hex(pre)||!found.begin()[3]||std::string(found.begin()[3])!=Hex(plan))throw std::runtime_error("Kithicor immutable intent differs");
  }else{
   if(KithicorPoisonCustody::Pending(fresh,lease.actor.character_id))throw std::runtime_error("Kithicor another pending intent differs");
   Q(fresh,abort_insert);
  }
  if(fresh.TransactionCommitStrict().Success())return;
 }catch(...){}std::this_thread::sleep_for(std::chrono::milliseconds(250));}
}
}

class Writer {
public:
 static bool Execute(Client& c,NPC& npc,unsigned chest) noexcept {
  auto& inv=c.GetInv();bool transaction=false,persistence_attempted=false,intent_known=false;
  std::unique_ptr<FungalInventory::UnknownInventory> hold;
  std::unique_ptr<ZoneDatabase> connection_owner;
  std::optional<Lease> retained_lease;std::string retained_key,retained_pre,retained_plan,abort_insert;
  try {
   connection_owner=std::make_unique<ZoneDatabase>();auto& db=*connection_owner;
   const auto* cfg=EQEmuConfig::get();if(!cfg||!db.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"kithicor-poison-exclusive"))return true;
   const auto run=c.GetSharedTaskId();if(run<=0)return true;
   Canonical(db,c,npc,uint64_t(run),chest);
   if(c.trade->state!=Trading)return true;
   auto ticket=c.InventorySessionReleaseTicket();auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
   if(!lease||!c.HasInventorySessionReservation())return true;
   EQ::ItemInstance* selected=nullptr;int16_t selected_slot=-1;
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(auto* p=inv.GetItem(s);p&&p->GetID()==38504){selected=p;selected_slot=s;break;}
   if(!selected||selected->GetSerialNumber()<=0||(selected->IsStackable()&&selected->GetCharges()<=0))return true;
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
    if(p==selected&&(!p->IsStackable()||p->GetCharges()==1))continue;
    std::unique_ptr<EQ::ItemInstance> copy(p->Clone());if(!copy)return true;
    if(p==selected)copy->SetCharges(p->GetCharges()-1);staged.push(copy.get());copy.release();
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
   hold=std::unique_ptr<FungalInventory::UnknownInventory>(new FungalInventory::UnknownInventory);
   hold->character_=c.CharacterID();hold->inventory_=&inv;
   const auto remember=[&](const auto& self,const EQ::ItemInstance* p)->void {if(!p)return;hold->items_.push_back(p);for(const auto& child:*p->GetContents())self(self,child.second);};
   for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& entry:*bucket)remember(remember,entry.second);
   for(auto* p:before_queue)remember(remember,p);for(auto* p:staged)remember(remember,p);
   uint8_t nonce[16];if(RAND_bytes(nonce,sizeof(nonce))!=1)return true;const auto key=Hex(nonce,sizeof(nonce));
   auto finish=std::make_unique<EQApplicationPacket>(OP_FinishTrade,0);
   Ack ack{&c,&npc,selected,uint64_t(run),zone->GetInstanceID(),chest,key,selected_slot,selected->GetSerialNumber(),selected->GetCharges(),false};
   std::vector<EQ::ItemInstance*> delivered{selected};
   std::unique_ptr<EQApplicationPacket> cursor_packet;
   if(!staged.empty()){const auto bytes=staged.peek_front()->Serialize(EQ::invslot::slotCursor);cursor_packet=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));auto* wire=reinterpret_cast<ItemPacket_Struct*>(cursor_packet->pBuffer);wire->PacketType=ItemPacketTrade;std::memcpy(wire->SerializedItem,bytes.data(),bytes.size());}
   auto connection_row=Q(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
   if(connection_row.RowCount()!=1||N(connection_row.begin()[1])||N(connection_row.begin()[2])!=1)return true;
   const auto connection=N(connection_row.begin()[0]);
   if(!db.TransactionBeginStrict().Success())return true;transaction=true;
   if(!Installed(db))throw std::runtime_error("Kithicor custody schema absent");Foundation(db);LockLease(db,*lease);
   auto housing=Q(db,"SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(c.CharacterID())+" FOR UPDATE");
   if(housing.RowCount()>1||(housing.RowCount()&&(N(housing.begin()[0])!=c.AccountID()||(housing.begin()[1]&&*housing.begin()[1]))))throw std::runtime_error("Kithicor other inventory custody pending");
   if(KithicorPoisonCustody::Pending(db,c.CharacterID()))throw std::runtime_error("Kithicor old custody pending");
   Canonical(db,c,npc,uint64_t(run),chest);ValidateTrees(db,c,before_queue,true);ValidateCurrency(db,c,c.GetPP());
   auto occupied=Q(db,fmt::format("SELECT status FROM kithicor_mission_actions WHERE shared_task_id={} AND phase=0 AND action_id={} FOR UPDATE",run,chest));
   if(occupied.RowCount())throw std::runtime_error("Kithicor chest already reserved/poisoned");
   const auto pre=Snapshot(db,c.CharacterID());
   // The immutable no-debit recovery plan returns the FULL original selected
   // vial as well as all other offers. It never reconstructs from a task label.
   ReturnPlan abort_plan;abort_plan.refund={{uint32_t(c.trade->pp),uint32_t(c.trade->gp),uint32_t(c.trade->sp),uint32_t(c.trade->cp)}};
   auto raw=InventoryRepository::GetLoadSnapshot(db,c.CharacterID(),true);if(!raw.success)throw std::runtime_error("Kithicor return plan source unavailable");
   const auto add_return=[&](uint32_t source){
    const auto found=std::find_if(raw.rows.begin(),raw.rows.end(),[&](const auto& row){return row.values.slot_id==source;});
    if(found==raw.rows.end())throw std::runtime_error("Kithicor return plan original row absent");
    auto row=Raw(*found);const auto index=abort_plan.rows.size();row.values.slot_id=index?EQ::invbag::CURSOR_BAG_BEGIN+index:EQ::invslot::slotCursor;
    abort_plan.rows.emplace_back(source,std::move(row));
   };
   for(size_t index=0;index<before_queue.size();++index)add_return(index?EQ::invbag::CURSOR_BAG_BEGIN+index:EQ::invslot::slotCursor);
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s)if(inv.GetItem(s))add_return(s);
   const auto abort_bytes=Encode(abort_plan);
   const auto intent_insert=fmt::format("INSERT INTO kithicor_poison_custody(request_key,character_id,account_id,inventory_epoch,inventory_token,process_key,client_key,instance_id,shared_task_id,phase,chest,npc_type_id,npc_entity_id,npc_spawn_id,source_slot,source_guid,source_charges,preimage,return_plan,postimage,physical,settled,settled_at,created_at) VALUES('{}',{},{},{},x'{}',x'{}',x'{}',{},{},0,{},{},{},{},{},{},{},x'{}',x'{}',NULL,0,0,NULL,UNIX_TIMESTAMP())",key,c.CharacterID(),c.AccountID(),lease->epoch,Hex(lease->token.data(),lease->token.size()),Hex(lease->owner.process_key),Hex(lease->owner.client_key),zone->GetInstanceID(),run,chest,npc.GetNPCTypeID(),npc.GetID(),npc.GetSpawnPointID(),selected_slot,selected->GetSerialNumber(),selected->GetCharges()>=0?selected->GetCharges():32767,Hex(pre),Hex(abort_bytes));
   abort_insert=intent_insert;
   const std::string normal_tail=",NULL,0,0,NULL,UNIX_TIMESTAMP())";
   const auto tail=abort_insert.rfind(normal_tail);if(tail==std::string::npos)throw std::runtime_error("Kithicor abort intent encoding unavailable");
   abort_insert.replace(tail,normal_tail.size(),",NULL,3,0,NULL,UNIX_TIMESTAMP())");
   retained_lease=*lease;retained_key=key;retained_pre=pre;retained_plan=abort_bytes;
   persistence_attempted=true;
   Q(db,fmt::format("INSERT INTO kithicor_mission_actions(shared_task_id,phase,action_id,character_id,item_id,status,updated_at) VALUES({},0,{},{},38504,0,UNIX_TIMESTAMP())",run,chest,c.CharacterID()));
   Q(db,intent_insert);
   Session(db,connection);if(!db.TransactionCommitStrict().Success())throw std::runtime_error("Kithicor intent commit unknown");transaction=false;intent_known=true;
   if(!db.TransactionBeginStrict().Success())throw std::runtime_error("Kithicor debit transaction unavailable");transaction=true;
   LockLease(db,*lease);Canonical(db,c,npc,uint64_t(run),chest);
   auto intent=Q(db,"SELECT request_key FROM kithicor_poison_custody WHERE request_key='"+key+"' AND physical=0 AND settled=0 AND "+LeaseWhere(*lease)+" FOR UPDATE");
   if(intent.RowCount()!=1||Snapshot(db,c.CharacterID())!=pre||!ticket->HeldFor(c))throw std::runtime_error("Kithicor intent physical preimage changed");
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
   auto outcome=Q(db,"UPDATE kithicor_poison_custody SET physical=1,postimage=x'"+Hex(post)+"' WHERE request_key='"+key+"' AND physical=0 AND settled=0 AND "+LeaseWhere(*lease));
   if(outcome.RowsAffected()!=1||db.TransactionStrictFailed())throw std::runtime_error("Kithicor physical outcome changed");
   Session(db,connection);if(!db.TransactionCommitStrict().Success())throw std::runtime_error("Kithicor physical commit unknown");transaction=false;
   // Allocation-free native ownership publication. staged now owns old cursor.
   inv.m_cursor.Swap(staged);std::array<std::unique_ptr<EQ::ItemInstance>,4> spent;
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<=EQ::invslot::TRADE_NPC_END;++s){auto at=inv.m_trade.find(s);if(at!=inv.m_trade.end()){spent[s-EQ::invslot::TRADE_BEGIN].reset(at->second);at->second=nullptr;}}
   c.GetPP()=after_pp;
   auto settled=Q(db,"UPDATE kithicor_poison_custody SET settled=1,settled_at=UNIX_TIMESTAMP() WHERE request_key='"+key+"' AND physical=1 AND settled=0 AND "+LeaseWhere(*lease));
   if(settled.RowsAffected()!=1)throw std::runtime_error("Kithicor memory publication acknowledgment unavailable");
   // New one-use gameplay proof is issued ONLY here after physical success.
   Canonical(db,c,npc,uint64_t(run),chest);
   struct Scope {Ack* old;Scope(Ack& a):old(current_ack){current_ack=&a;}~Scope(){current_ack=old;}} scope(ack);
   const bool updated=c.UpdateTasksOnDeliver(delivered,*c.trade,&npc);
   // Neither consuming the capability nor executing the callback proves task
   // acceptance. Unknown/missing SQL stays physical-only; never return another
   // vial or replay native credit to guess the missing gameplay outcome.
   bool gameplay_accepted=false;
   try{auto receipt=Q(db,fmt::format("SELECT status FROM kithicor_mission_actions WHERE shared_task_id={} AND phase=0 AND action_id={} AND character_id={} AND item_id=38504 AND status=1",run,chest,c.CharacterID()));gameplay_accepted=updated&&receipt.RowCount()==1;}catch(...){}
   if(!gameplay_accepted)c.Message(Chat::Yellow,"Your vial was used, but task progress could not be confirmed.");
   c.trade->Reset();c.QueuePacket(finish.get());if(cursor_packet)c.QueuePacket(cursor_packet.get());c.SendMoneyUpdate();c.RecalcWeight();
   return true;
  }catch(...) {
   if(transaction&&connection_owner)connection_owner->TransactionRollbackStrict(); // NO inference of known rollback.
   connection_owner.reset(); // End this exclusive writer before fresh observation.
   if(persistence_attempted&&hold){
    hold->Attach();hold.release();
    if(!intent_known&&retained_lease)EnsureAbortIntent(*retained_lease,retained_key,retained_pre,retained_plan,abort_insert);
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
