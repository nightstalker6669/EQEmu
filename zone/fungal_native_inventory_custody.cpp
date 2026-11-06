// Finite source-only Fungal writer. Spells r4 actual original-event/World
// seams and uncertain-session hold must be composed before this can be enabled.
#include "fungal_item_operation_source.h"
#include "../common/fungal_item_custody_sql.h"
#include "../common/fungal_unknown_inventory.h"
#include "../common/util/uuid.h"
#include "client.h"
#include "zonedb.h"
extern ZoneDatabase database;
#include "inventory_session_owner.h"
#include "../common/shared_core/inventory_row_db.h"
#include "../common/shared_core/inventory_projection.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>
#include <cstring>
namespace FungalInventory {
namespace {
using Row=EQEmu::SharedCore::InventoryRow;
using Projection=EQEmu::SharedCore::InventoryProjection::ReconstructionScope;
std::string Hex(const void* data,size_t size) {
 const auto* p=static_cast<const uint8_t*>(data);const char* h="0123456789ABCDEF";
 std::string s;s.reserve(size*2);for(size_t i=0;i<size;++i){s+=h[p[i]>>4];s+=h[p[i]&15];}return s;
}
std::string Hex(const std::string& s){return Hex(s.data(),s.size());}
uint64_t Number(const char* p){if(!p)throw std::runtime_error("Fungal null identity");std::string s=p;size_t n=0;auto v=std::stoull(s,&n);if(n!=s.size())throw std::runtime_error("Fungal invalid identity");return v;}
auto Query(ZoneDatabase& db,const std::string& sql) {
 if(db.GetStatus()!=DBcore::Connected)throw std::runtime_error("Fungal disconnected");
 auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Fungal strict query failed");return r;
}
std::string Key(uint8_t scope,uint32_t task,uint64_t occurrence,uint8_t activity) {
 return "scope="+std::to_string(scope)+" AND task_id="+std::to_string(task)+" AND native_occurrence="+std::to_string(occurrence)+" AND activity_id="+std::to_string(activity);
}
Row OutputRow(uint32_t who,int16_t slot,EQ::ItemInstance& item) {
 Row r;auto& v=r.values;v.character_id=who;v.slot_id=slot;v.item_id=item.GetID();v.charges=item.GetCharges();v.color=item.GetColor();v.instnodrop=item.IsAttuned();v.custom_data=item.GetCustomDataString();v.ornament_icon=item.GetOrnamentationIcon();v.ornament_idfile=item.GetOrnamentationIDFile();v.ornament_hero_model=item.GetOrnamentHeroModel();v.guid=item.GetSerialNumber();return r;
}
}
std::unique_ptr<FungalItemSource::CommittedOperation> NativeCustody::Execute(Client& c,std::unique_ptr<FungalItemSource::OriginalOperation> original) noexcept {
 using namespace FungalItemSource;
 bool transaction=false,commit_attempted=false,known_commit=false;
 std::unique_ptr<UnknownInventory> unknown;
 auto& db=database;auto& inv=c.GetInv();
 if(!original)return nullptr;
 try {
 const auto& f=original->facts_;
 const auto live=[&](){return NativeEvents::Revalidate(c,*original);};
 if(!live())return nullptr;
 struct View {
  uint8_t scope_,activity_;uint32_t task_,accepted_,character_,account_;uint64_t occurrence_;
  uint32_t actor_entity_,actor_spawn_,actor_npc_;std::vector<char> source_token_;std::string source_identity_;
 } e{f.scope,uint8_t(f.activity),f.task,f.accepted,f.character,c.AccountID(),f.occurrence,f.npc_entity,f.npc_spawn,f.npc_type,EQ::Util::UUID::Generate().ToByteArray(),"source_sequence="+std::to_string(f.source_sequence)};
 if(!f.source_sequence||f.activity>255||e.source_token_.size()!=16)return nullptr;
 const bool personal=e.scope_==1&&e.task_==920141&&e.activity_==7&&e.actor_npc_==481093;
 const bool spores=e.scope_==2&&e.task_==920143&&e.activity_==5&&e.actor_npc_==481093;
 const bool bottle=e.scope_==2&&e.task_==920143&&e.activity_==10&&e.actor_npc_==481052;
 if((!personal&&!spores&&!bottle)||!e.occurrence_||!e.accepted_||!e.actor_entity_||!e.actor_spawn_||e.character_!=c.CharacterID()||e.account_!=c.AccountID()||e.source_identity_.empty()||e.source_identity_.size()>2048||std::all_of(e.source_token_.begin(),e.source_token_.end(),[](char v){return !v;}))return nullptr;
 auto reservation=c.InventorySessionReleaseTicket();
 auto lease=reservation?reservation->SnapshotFor(c):std::nullopt;
 if(!lease||!lease->active||lease->actor.character_id!=e.character_||lease->actor.account_id!=e.account_||!c.HasInventorySessionReservation())return nullptr;
 const uint32_t output=personal?40997:spores?43586:43587;const uint32_t quantity=spores?2:1;
 if(f.output_item!=output||f.output_quantity!=quantity||f.input_item!=(spores?43585u:0u)||f.input_quantity!=(spores?2u:0u)||original->trade_!=spores)return nullptr;
 struct Take {int16_t slot;EQ::ItemInstance** live;EQ::ItemInstance* original;std::unique_ptr<EQ::ItemInstance> remainder;int16_t debit;};
 std::vector<Take> takes;
 unknown=std::unique_ptr<UnknownInventory>(new UnknownInventory);
 unknown->character_=c.CharacterID();unknown->inventory_=&inv;
 const auto remember=[&](const auto& self,const EQ::ItemInstance* item)->void {if(!item)return;unknown->items_.push_back(item);for(const auto& child:*item->GetContents())self(self,child.second);};
 for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& x:*bucket)remember(remember,x.second);
 for(auto it=inv.cursor_cbegin();it!=inv.cursor_cend();++it)remember(remember,*it);
 std::unique_ptr<EQ::ItemInstance> prepared_output;
 EQ::ItemInstance** output_live=nullptr;
 {
  const auto* definition=db.GetItem(output);
  if(!definition||definition->ItemClass!=0||!definition->Stackable||definition->StackSize<quantity||definition->MaxCharges!=0||c.CheckLoreConflict(definition))return nullptr;
  prepared_output.reset(db.CreateItem(definition,int16_t(quantity)));
  if(!prepared_output||prepared_output->GetSerialNumber()<=0||prepared_output->GetCharges()!=int16_t(quantity)||!prepared_output->GetContents()->empty())return nullptr;
  int16_t slot=-1;
  for(int16_t s=EQ::invslot::GENERAL_BEGIN;s<=EQ::invslot::GENERAL_END;++s) {
   if(inv.GetLookup()&&((uint64_t(1)<<s)&inv.GetLookup()->PossessionsBitmask)&&!inv.GetItem(s)){slot=s;break;}
  }
  // Supported placement is one empty ordinary general root. No cursor/stack
  // merge or invented owed award. Full inventory rejects with property intact.
  if(slot<0)return nullptr;
  output_live=&inv.m_inv.emplace(slot,nullptr).first->second;
  if(*output_live)return nullptr;
  if(spores) {
   int remaining=2;
   for(int16_t s=EQ::invslot::TRADE_BEGIN;s<EQ::invslot::TRADE_BEGIN+4&&remaining;++s) {
    auto at=inv.m_trade.find(s);if(at==inv.m_trade.end()||!at->second||at->second->GetID()!=43585)continue;
    auto* item=at->second;
    if(!item->GetItem()||!item->IsStackable()||item->GetCharges()<=0||item->GetSerialNumber()<=0||!item->GetContents()->empty())return nullptr;
    const auto debit=int16_t(std::min(remaining,int(item->GetCharges())));
    std::unique_ptr<EQ::ItemInstance> remainder;
    if(item->GetCharges()>debit){remainder.reset(item->Clone());if(!remainder)return nullptr;remainder->SetCharges(item->GetCharges()-debit);}
    takes.push_back({s,&at->second,item,std::move(remainder),debit});remaining-=debit;
   }
   if(remaining)return nullptr; // no partial debit/output/progress
  }
  const auto token=Hex(e.source_token_.data(),e.source_token_.size());
  auto receipt=std::unique_ptr<CommittedOperation>(new CommittedOperation(*original,0));
  const auto output_row=OutputRow(e.character_,slot,*prepared_output);
  if(!EQEmu::SharedCore::InventoryProjection::Matches(output_row,*prepared_output,Projection::CharacterProfile))return nullptr;
  const auto output_identity=EQEmu::SharedCore::InventoryRowDB::Values(output_row,slot);
  std::vector<std::unique_ptr<EQApplicationPacket>> packets;packets.reserve(takes.size()+1);
  const auto packet=[&](int16_t changed,const EQ::ItemInstance* item) {
   if(item){const auto bytes=item->Serialize(changed);auto p=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));auto* wire=reinterpret_cast<ItemPacket_Struct*>(p->pBuffer);wire->PacketType=ItemPacketTrade;std::memcpy(wire->SerializedItem,bytes.data(),bytes.size());packets.push_back(std::move(p));}
   else {auto p=std::make_unique<EQApplicationPacket>(OP_MoveItem,sizeof(MoveItem_Struct));auto* wire=reinterpret_cast<MoveItem_Struct*>(p->pBuffer);wire->from_slot=changed;wire->to_slot=0xFFFFFFFF;wire->number_in_stack=0xFFFFFFFF;packets.push_back(std::move(p));}
  };
  packet(slot,prepared_output.get());for(const auto& take:takes)packet(take.slot,take.remainder.get());
  auto before=Query(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(before.RowCount()!=1||Number(before.begin()[1])||Number(before.begin()[2])!=1)return nullptr;
  const auto connection=Number(before.begin()[0]);
  if(!db.TransactionBeginStrict().Success())return nullptr;transaction=true;
  auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN('inventory','character_data','housing_inventory_sessions','housing_live_item_leases','fungal_item_operations')");
  if(engines.RowCount()!=1||Number(engines.begin()[0])!=5)throw std::runtime_error("Fungal table foundation differs");
  auto actor=Query(db,"SELECT id FROM character_data WHERE id="+std::to_string(e.character_)+" AND account_id="+std::to_string(e.account_)+" AND deleted_at IS NULL FOR UPDATE");if(actor.RowCount()!=1)throw std::runtime_error("Fungal actor differs");
  auto held=Query(db,"SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(e.character_)+" FOR UPDATE");
  if(held.RowCount()!=1)throw std::runtime_error("Fungal lease absent");auto h=held.begin();
  if(Number(h[0])!=lease->actor.account_id||Number(h[1])!=lease->epoch||!h[2]||std::string(h[2])!=Hex(lease->token.data(),lease->token.size())||!h[3]||std::string(h[3])!=Hex(lease->owner.process_key)||!h[4]||std::string(h[4])!=Hex(lease->owner.client_key)||Number(h[5])!=lease->owner.zone_id||Number(h[6])!=lease->owner.instance_id||Number(h[7])!=1)throw std::runtime_error("Fungal lease changed");
  auto custody=Query(db,"SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(e.character_)+" FOR UPDATE");
  if(custody.RowCount()>1||(custody.RowCount()&&(Number(custody.begin()[0])!=e.account_||(custody.begin()[1]&&*custody.begin()[1]))))throw std::runtime_error("Fungal pending custody");
  if(!reservation->HeldFor(c)||!live())throw std::runtime_error("Fungal original owner changed");
  const auto key=Key(e.scope_,e.task_,e.occurrence_,e.activity_);
  if(e.scope_==2)FungalCustodySQL::LockShared(db,e.occurrence_,e.accepted_,e.character_,e.activity_,false);
  else FungalCustodySQL::LockPersonal(db,e.occurrence_,e.accepted_,e.character_,f.personal_slot);
  auto existing=Query(db,"SELECT outcome_id FROM fungal_item_operations WHERE "+key+" FOR UPDATE");
  // Existing physical outcome is never a fresh award or a new capability.
  // Source must resolve its original pending publication; do not regrant.
  if(existing.RowCount()){db.TransactionRollbackStrict();transaction=false;return nullptr;}
  auto empty=Query(db,"SELECT slot_id FROM inventory WHERE character_id="+std::to_string(e.character_)+" AND(slot_id="+std::to_string(slot)+" OR slot_id BETWEEN "+std::to_string(EQ::InventoryProfile::CalcSlotId(slot,uint8_t(0)))+" AND "+std::to_string(EQ::InventoryProfile::CalcSlotId(slot,uint8_t(0))+EQ::invbag::SLOT_COUNT-1)+") FOR UPDATE");
  if(empty.RowCount()||*output_live)throw std::runtime_error("Fungal output preimage changed");
  std::string source=e.source_identity_;
  for(auto& take:takes) {
   auto rows=Query(db,"SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(e.character_)+" AND slot_id="+std::to_string(take.slot)+" FOR UPDATE");
   if(rows.RowCount()!=1||*take.live!=take.original)throw std::runtime_error("Fungal offered identity changed");
   auto row=EQEmu::SharedCore::InventoryRowDB::Decode(rows.begin());
   if(!EQEmu::SharedCore::InventoryProjection::Matches(row,*take.original,Projection::CharacterProfile))throw std::runtime_error("Fungal original property differs");
   source+='\n'+EQEmu::SharedCore::InventoryRowDB::Values(row,take.slot)+";debit="+std::to_string(take.debit);
  }
  if(source.size()>4096||!live()||!reservation->HeldFor(c))throw std::runtime_error("Fungal final source fence differs");
  for(auto& take:takes)if(!db.SaveInventory(e.character_,take.remainder.get(),take.slot))throw std::runtime_error("Fungal raw-spore save failed");
  if(!db.SaveInventory(e.character_,prepared_output.get(),slot))throw std::runtime_error("Fungal output save failed");
  auto inserted=Query(db,"INSERT INTO fungal_item_operations(scope,task_id,native_occurrence,activity_id,accepted_time,recipient_character,source_token,actor_entity,actor_spawn,actor_npc,source_identity,output_item,output_quantity,output_serial,output_slot,output_charges,output_identity,native_ack) VALUES("+std::to_string(e.scope_)+","+std::to_string(e.task_)+","+std::to_string(e.occurrence_)+","+std::to_string(e.activity_)+","+std::to_string(e.accepted_)+","+std::to_string(e.character_)+",x'"+token+"',"+std::to_string(e.actor_entity_)+","+std::to_string(e.actor_spawn_)+","+std::to_string(e.actor_npc_)+",x'"+Hex(source)+"',"+std::to_string(output)+","+std::to_string(quantity)+","+std::to_string(prepared_output->GetSerialNumber())+","+std::to_string(slot)+","+std::to_string(prepared_output->GetCharges())+",x'"+Hex(output_identity)+"',0)");
  if(inserted.RowsAffected()!=1)throw std::runtime_error("Fungal once outcome not inserted");
  auto final=Query(db,"SELECT CONNECTION_ID(),@@in_transaction");
  if(final.RowCount()!=1||Number(final.begin()[0])!=connection||Number(final.begin()[1])!=1||db.TransactionStrictFailed()||!live())throw std::runtime_error("Fungal strict transaction changed");
  auto identity=Query(db,"SELECT LAST_INSERT_ID()");if(identity.RowCount()!=1||!Number(identity.begin()[0]))throw std::runtime_error("Fungal outcome ID absent");receipt->outcome_=Number(identity.begin()[0]);
  commit_attempted=true;auto commit=db.TransactionCommitStrict();transaction=false;
  if(!commit.Success())throw std::runtime_error("Fungal commit unknown");known_commit=true;
  // Allocation-free publication into preallocated native map values. The old
  // raw instances stay owned by temporary unique_ptrs until normal teardown.
  *output_live=prepared_output.release();
  for(auto& take:takes){auto* spent=*take.live;*take.live=take.remainder.release();take.remainder.reset(spent);}
  // Source consumes its private receipt and marks published_ exactly once.
  for(auto& p:packets){auto* send=p.release();try{c.FastQueuePacket(&send);}catch(...){/* Physical outcome remains known; delivery is not a new award. */}safe_delete(send);}
  return receipt;
 }
 }catch(...) {
  if(transaction)db.TransactionRollbackStrict();
  if(commit_attempted&&!known_commit&&unknown){unknown->Attach();unknown.release();}
  return nullptr;
 }
}

void NativeCustody::PersonalStageSaved(Database& db,const FungalItemSource::CommittedOperation& proof,bool saved) noexcept {
 const auto& f=proof.facts_;
 if(!saved||!proof.published_||!proof.outcome_||f.scope!=1||f.task!=920141||f.activity!=7)return;
 try {
  using FungalCustodySQL::Query;
  // Actual observed original native SaveClientState receipt only. It never
  // creates an operation, grants another item, or updates progress directly.
  Query(db,"UPDATE fungal_item_operations o SET o.native_ack=1 WHERE o.outcome_id="+std::to_string(proof.outcome_)+" AND o.scope=1 AND o.task_id=920141 AND o.native_occurrence="+std::to_string(f.occurrence)+" AND o.activity_id=7 AND o.accepted_time="+std::to_string(f.accepted)+" AND o.recipient_character="+std::to_string(f.character)+" AND o.output_item=40997 AND o.output_quantity=1 AND o.native_ack=0 AND EXISTS(SELECT 1 FROM character_task_reward_instances r WHERE r.character_id=o.recipient_character AND r.task_id=o.task_id AND r.occurrence_id=o.native_occurrence AND r.accepted_time=o.accepted_time) AND EXISTS(SELECT 1 FROM character_tasks t JOIN character_activities a ON a.charid=t.charid AND a.taskid=t.taskid WHERE t.charid=o.recipient_character AND t.taskid=920141 AND t.type=2 AND t.slot="+std::to_string(f.personal_slot)+" AND t.acceptedtime=o.accepted_time AND a.activityid=7 AND a.donecount>=1 AND a.completed=1)");
 }catch(...){} // ACK remains unresolved; no replay/regrant or stage inference.
}
bool NativeCustody::KnownWorldStage(Database& db,uint64_t run,uint64_t outcome,uint32_t accepted,uint32_t recipient,uint32_t activity,uint32_t count) noexcept {
 if(!run||!outcome||!accepted||!recipient||(activity!=5&&activity!=10)||count!=(activity==5?2u:1u))return false;
 try {
  auto r=FungalCustodySQL::Query(db,"SELECT o.outcome_id FROM fungal_item_operations o JOIN shared_tasks t ON t.id=o.native_occurrence JOIN shared_task_activity_state a ON a.shared_task_id=t.id AND a.activity_id=o.activity_id WHERE o.outcome_id="+std::to_string(outcome)+" AND o.scope=2 AND o.task_id=920143 AND o.native_occurrence="+std::to_string(run)+" AND o.activity_id="+std::to_string(activity)+" AND o.accepted_time="+std::to_string(accepted)+" AND o.recipient_character="+std::to_string(recipient)+" AND o.output_item="+std::to_string(activity==5?43586:43587)+" AND o.output_quantity="+std::to_string(count)+" AND o.native_ack=1 AND t.task_id=920143 AND t.accepted_time=FROM_UNIXTIME("+std::to_string(accepted)+") AND a.done_count="+std::to_string(count)+" AND a.completed_time IS NOT NULL");return r.RowCount()==1;
 }catch(...){return false;}
}
}
