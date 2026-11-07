#include "recharge_recipe8.h"
#include "client.h"
#include "object.h"
#include "zonedb.h"
#include "zone.h"
#include "inventory_session_owner.h"
#include "../common/fungal_unknown_inventory.h"
#include "../common/fire_native_inventory_fence.h"
#include "../common/shared_core/inventory_row_db.h"
#include "../common/shared_core/inventory_projection.h"
#include "../common/housing/evolving_item_custody_db.h"
#include <algorithm>
#include <array>
#include <optional>
#include <utility>
#include <cstring>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
extern ZoneDatabase database;
namespace RechargeRecipe8 {
thread_local Proof* Custody::current_=nullptr;
Proof::Proof(Client& c,uint32_t recipe):client_(&c),recipe_(recipe),character_(c.CharacterID()),account_(c.AccountID()){}
namespace {
using Row=EQEmu::SharedCore::InventoryRow;
using Projection=EQEmu::SharedCore::InventoryProjection::ReconstructionScope;
namespace Evo=Housing::EvolvingItemCustody;
uint64_t Number(const char* p){if(!p)throw std::runtime_error("Missing recharge identity");size_t n=0;std::string s=p;auto v=std::stoull(s,&n);if(n!=s.size())throw std::runtime_error("Malformed recharge identity");return v;}
auto Query(const std::string& sql){if(database.GetStatus()!=DBcore::Connected)throw std::runtime_error("Recharge database disconnected");auto r=database.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Strict recharge query failed");return r;}
std::string Hex(const void* data,size_t size){const char* h="0123456789ABCDEF";const auto* p=static_cast<const uint8_t*>(data);std::string s;s.reserve(size*2);for(size_t i=0;i<size;++i){s+=h[p[i]>>4];s+=h[p[i]&15];}return s;}
std::string Hex(const std::string& s){return Hex(s.data(),s.size());}
bool Shape(const Definition& d,const DBTradeskillRecipe_Struct& s){return s.recipe_id==d.recipe&&uint32_t(s.tradeskill)==57&&s.nofail&&s.trivial==d.trivial&&!s.skill_needed&&!s.quest&&!s.replace_container&&!s.must_learn&&s.onfail.empty()&&s.onsuccess.size()==1&&s.onsuccess.begin()->first==d.output&&s.onsuccess.begin()->second==1;}
bool Actor(Client& c){return c.GetPP().race==12;}
bool Kit(const EQ::ItemInstance* kit){const auto* data=kit&&kit->IsClassBag()?kit->GetItem():nullptr;return data&&data->ID==17902&&data->BagType==10&&data->BagSlots==8&&data->BagSize==4;}
using Serial=decltype(std::declval<EQ::ItemInstance&>().GetSerialNumber());
struct Part {uint8_t child;int16_t slot;EQ::ItemInstance* item;uint32_t id;Serial serial;int32_t charges;bool device;Row row;};
struct Role {uint32_t success=0,fail=0,component=0,container=0;};
}
std::unique_ptr<Proof> Custody::Manual(Client& c,DBTradeskillRecipe_Struct& spec,Object* world,EQ::ItemInstance* kit,int16_t parent_slot)noexcept {
 bool transaction=false,attempted=false,known=false;
 std::unique_ptr<FungalInventory::UnknownInventory> unknown;
 auto& inv=c.GetInv();
 try {
  const auto* found=Find(spec.recipe_id);if(!found||world||!Shape(*found,spec)||!Actor(c)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&inv))return nullptr;
  const auto& d=*found;
  if(!EQ::ValueWithin(parent_slot,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END)||!inv.GetLookup()||!((uint64_t(1)<<parent_slot)&inv.GetLookup()->PossessionsBitmask))return nullptr;
  auto* current=inv.GetItem(parent_slot);if(!current||current!=kit||!Kit(current)||current->GetSerialNumber()<=0)return nullptr;
  const auto who=c.CharacterID();const auto account=c.AccountID();const auto kit_serial=kit->GetSerialNumber();
  auto reservation=c.InventorySessionReleaseTicket();auto lease=reservation?reservation->SnapshotFor(c):std::nullopt;
  if(!who||!account||!lease||!lease->active||!c.HasInventorySessionReservation()||lease->actor.character_id!=who||lease->actor.account_id!=account)return nullptr;
  std::map<uint32_t,uint32_t> wanted,seen;for(const auto& cost:d.costs)if(cost.item)wanted[cost.item]+=cost.units;
  std::vector<Part> parts;parts.reserve(8);std::set<Serial> serials{kit_serial};EQ::ItemInstance* device=nullptr;uint8_t device_child=0;
  for(const auto& entry:kit->m_contents){
   auto* item=entry.second;const auto* data=item?item->GetItem():nullptr;
   if(entry.first>=8||!data||!item->IsClassCommon()||item->GetSerialNumber()<=0||!serials.insert(item->GetSerialNumber()).second||data->Size>4)return nullptr;
   const bool target=item->GetID()==d.input;
   if(target){if(device||item->IsStackable()||item->IsScaling()||item->m_scaledItem||item->IsEvolving()||data->CharmFileID||data->MaxCharges!=d.input_cap||item->GetCharges()<0||uint32_t(item->GetCharges())>d.input_cap)return nullptr;device=item;device_child=entry.first;}
   else {
    if(!wanted.count(item->GetID())||!item->GetContents()->empty()||item->IsScaling()||item->m_scaledItem||item->IsEvolving())return nullptr;
    if(item->IsStackable()&&item->GetCharges()<=0)return nullptr;
    seen[item->GetID()]+=item->IsStackable()?uint32_t(item->GetCharges()):1u;
   }
   const auto absolute_slot=EQ::InventoryProfile::CalcSlotId(parent_slot,entry.first);if(absolute_slot<0)return nullptr;
   parts.push_back(Part{uint8_t(entry.first),int16_t(absolute_slot),item,item->GetID(),item->GetSerialNumber(),item->GetCharges(),target,{}});
  }
  if(!device||seen!=wanted)return nullptr; // exact manual physical inputs; no excess stack debit
  const auto* output=database.GetItem(d.output);if(!output||output->ItemClass!=0||output->Stackable||output->MaxCharges!=d.charges||output->EvolvingItem||output->CharmFileID||output->LoreGroup)return nullptr;
  for(unsigned i=0;i<6;++i)if(device->GetItem()->AugSlotType[i]!=output->AugSlotType[i])return nullptr;
  const auto original=[&](){
   if(!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&inv)||!reservation->HeldFor(c)||c.CharacterID()!=who||c.AccountID()!=account||!Actor(c))return false;
   auto* source=inv.GetItem(parent_slot);if(!source||source!=kit||!Kit(source)||source->GetSerialNumber()!=kit_serial||source->m_contents.size()!=parts.size())return false;
   for(const auto& p:parts){auto at=source->m_contents.find(p.child);if(at==source->m_contents.end()||at->second!=p.item||!at->second||at->second->GetID()!=p.id||at->second->GetSerialNumber()!=p.serial||at->second->GetCharges()!=p.charges)return false;}
   return Shape(d,spec);
  };
  if(!original())return nullptr;
  auto copy_delivered=[&](const auto& self,const EQ::ItemInstance& from,EQ::ItemInstance& to)->void{to.m_task_delivered_count=from.m_task_delivered_count;for(const auto& x:from.m_contents){auto at=to.m_contents.find(x.first);if(!x.second||at==to.m_contents.end()||!at->second)throw std::runtime_error("Recharge clone tree differs");self(self,*x.second,*at->second);}};
  std::unique_ptr<EQ::ItemInstance> prepared(device->Clone());if(!prepared)return nullptr;copy_delivered(copy_delivered,*device,*prepared);
  // Self recharge keeps the owned definition. Only the two prescribed Full/Half conversions replace it.
  if(d.input!=d.output){auto definition=std::make_unique<EQ::ItemData>(*output);delete prepared->m_item;prepared->m_item=definition.release();}
  prepared->SetCharges(int16_t(d.charges));
  auto proof=std::unique_ptr<Proof>(new Proof(c,d.recipe));
  unknown=std::unique_ptr<FungalInventory::UnknownInventory>(new FungalInventory::UnknownInventory);unknown->character_=who;unknown->inventory_=&inv;
  const auto remember=[&](const auto& self,const EQ::ItemInstance* item)->void{if(!item)return;unknown->items_.push_back(item);for(const auto& x:*item->GetContents())self(self,x.second);};
  for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& x:*bucket)remember(remember,x.second);for(auto at=inv.cursor_cbegin();at!=inv.cursor_cend();++at)remember(remember,*at);
  std::vector<std::unique_ptr<EQ::ItemInstance>> spent;spent.reserve(parts.size());
  auto state=Query("SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");if(state.RowCount()!=1||Number(state.begin()[1])||Number(state.begin()[2])!=1)return nullptr;const auto connection=Number(state.begin()[0]);if(!database.TransactionBeginStrict().Success())return nullptr;transaction=true;
  auto engines=Query("SELECT COUNT(*)FROM information_schema.tables WHERE table_schema=DATABASE()AND engine='InnoDB'AND table_name IN('inventory','character_data','housing_inventory_sessions','housing_live_item_leases','character_evolving_items','housing_evolving_item_locations','tradeskill_recipe','tradeskill_recipe_entries','items','items_evolving_details')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=10)throw std::runtime_error("Recharge strict foundation differs");
  auto actor=Query("SELECT id FROM character_data WHERE id="+std::to_string(who)+" AND account_id="+std::to_string(account)+" AND deleted_at IS NULL FOR UPDATE");if(actor.RowCount()!=1)throw std::runtime_error("Recharge actor changed");
  auto held=Query("SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(who)+" FOR UPDATE");if(held.RowCount()!=1)throw std::runtime_error("Recharge lease absent");auto h=held.begin();
  if(Number(h[0])!=account||Number(h[1])!=lease->epoch||!h[2]||std::string(h[2])!=Hex(lease->token.data(),lease->token.size())||!h[3]||std::string(h[3])!=Hex(lease->owner.process_key)||!h[4]||std::string(h[4])!=Hex(lease->owner.client_key)||Number(h[5])!=lease->owner.zone_id||Number(h[6])!=lease->owner.instance_id||Number(h[7])!=1)throw std::runtime_error("Recharge lease changed");
  auto pending=Query("SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(who)+" FOR UPDATE");if(pending.RowCount()>1||(pending.RowCount()&&(Number(pending.begin()[0])!=account||(pending.begin()[1]&&*pending.begin()[1]))))throw std::runtime_error("Recharge pending custody");
  auto recipe=Query("SELECT id FROM tradeskill_recipe WHERE id="+std::to_string(d.recipe)+" AND tradeskill=57 AND nofail=1 AND trivial="+std::to_string(d.trivial)+" AND skillneeded=0 AND quest=0 AND replace_container=0 AND must_learn=0 AND learned_by_item_id=0 AND enabled=1 AND min_expansion=19 AND max_expansion=-1 AND content_flags IS NULL AND content_flags_disabled IS NULL FOR UPDATE");if(recipe.RowCount()!=1)throw std::runtime_error("Recharge recipe changed");
  std::map<uint32_t,Role> expected;expected[d.input].component=1;expected[d.output].success=1;expected[17902].container=1;for(const auto& cost:d.costs)if(cost.item)expected[cost.item].component+=cost.units;
  auto roles=Query("SELECT item_id,successcount,failcount,componentcount,iscontainer FROM tradeskill_recipe_entries WHERE recipe_id="+std::to_string(d.recipe)+" ORDER BY item_id FOR UPDATE");if(roles.RowCount()!=expected.size())throw std::runtime_error("Recharge role universe changed");
  for(auto at=roles.begin();at!=roles.end();++at){auto e=expected.find(uint32_t(Number(at[0])));if(e==expected.end()||Number(at[1])!=e->second.success||Number(at[2])!=e->second.fail||Number(at[3])!=e->second.component||Number(at[4])!=e->second.container)throw std::runtime_error("Recharge exact roles differ");}
  auto definitions=Query("SELECT id,itemclass,maxcharges,stackable FROM items WHERE id IN("+std::to_string(d.input)+","+std::to_string(d.output)+") ORDER BY id FOR UPDATE");if(definitions.RowCount()!=(d.input==d.output?1u:2u))throw std::runtime_error("Recharge definitions absent");for(auto at=definitions.begin();at!=definitions.end();++at)if(Number(at[1])||Number(at[2])!=(Number(at[0])==d.output?d.charges:d.input_cap)||Number(at[3]))throw std::runtime_error("Recharge native charge definition differs");
  auto parent_rows=Query("SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(parent_slot)+" FOR UPDATE");if(parent_rows.RowCount()!=1)throw std::runtime_error("Recharge owned toolbox row absent");auto parent=EQEmu::SharedCore::InventoryRowDB::Decode(parent_rows.begin());if(parent.values.guid!=uint64_t(kit_serial))throw std::runtime_error("Original toolbox GUID differs");
  const auto first=EQ::InventoryProfile::CalcSlotId(parent_slot,uint8_t(0));auto child_rows=Query("SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id BETWEEN "+std::to_string(first)+" AND "+std::to_string(first+EQ::invbag::SLOT_COUNT-1)+" ORDER BY slot_id FOR UPDATE");if(child_rows.RowCount()!=parts.size())throw std::runtime_error("Recharge child row universe differs");
  Row before;int16_t device_slot=-1;
  for(auto at=child_rows.begin();at!=child_rows.end();++at){auto row=EQEmu::SharedCore::InventoryRowDB::Decode(at);auto part=std::find_if(parts.begin(),parts.end(),[&](const auto& p){return uint32_t(p.slot)==row.values.slot_id;});if(part==parts.end()||row.values.guid!=uint64_t(part->serial))throw std::runtime_error("Recharge original child GUID differs");row.evolving_payload=Evo::Capture(database,who,part->slot,*part->item);if(!EQEmu::SharedCore::InventoryProjection::Matches(row,*part->item,Projection::CharacterProfile))throw std::runtime_error("Recharge original child property differs");part->row=row;if(part->device){before=row;device_slot=part->slot;}auto child=row;child.values.slot_id=part->child;parent.children.push_back(std::move(child));}
  if(device_slot<0||!EQEmu::SharedCore::InventoryProjection::Matches(parent,*kit,Projection::CharacterProfile))throw std::runtime_error("Recharge full original toolbox differs");
  for(const auto& component:Evo::Decode(before.evolving_payload)){auto locator=Query("SELECT inventory_guid FROM housing_evolving_item_locations WHERE evolving_id="+std::to_string(component.id)+" FOR UPDATE");if(locator.RowCount()>1||(locator.RowCount()==1&&Number(locator.begin()[0])!=before.values.guid))throw std::runtime_error("Recharge original evolving locator GUID differs");}
  auto after=before;after.values.item_id=d.output;after.values.charges=uint16_t(d.charges);after.null_fields&=uint8_t(~EQEmu::SharedCore::NullCharges);prepared->m_color=EQEmu::SharedCore::InventoryProjection::Color(after,*prepared->GetItem());
  if(prepared->GetSerialNumber()!=device->GetSerialNumber()||!EQEmu::SharedCore::InventoryProjection::Matches(after,*prepared,Projection::CharacterProfile))throw std::runtime_error("Recharge loses owned property");
  auto after_parent=parent;after_parent.children.clear();auto retained=after;retained.values.slot_id=device_child;after_parent.children.push_back(retained);
  std::unique_ptr<EQ::ItemInstance> wire(kit->Clone());if(!wire)throw std::runtime_error("Recharge packet clone absent");copy_delivered(copy_delivered,*kit,*wire);
  for(auto at=wire->m_contents.begin();at!=wire->m_contents.end();){if(at->first!=device_child){delete at->second;at=wire->m_contents.erase(at);}else ++at;}
  auto wire_device=std::unique_ptr<EQ::ItemInstance>(prepared->Clone());if(!wire_device)throw std::runtime_error("Recharge packet device absent");copy_delivered(copy_delivered,*prepared,*wire_device);auto w=wire->m_contents.find(device_child);if(w==wire->m_contents.end())throw std::runtime_error("Recharge packet child absent");delete w->second;w->second=wire_device.release();
  if(wire->GetSerialNumber()!=kit_serial||!EQEmu::SharedCore::InventoryProjection::Matches(after_parent,*wire,Projection::CharacterProfile))throw std::runtime_error("Recharge packet postimage differs");
  const auto bytes=wire->Serialize(parent_slot);auto packet=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));auto* body=reinterpret_cast<ItemPacket_Struct*>(packet->pBuffer);body->PacketType=ItemPacketTrade;std::memcpy(body->SerializedItem,bytes.data(),bytes.size());
  if(!original()||!EQEmu::SharedCore::InventoryProjection::Matches(parent,*kit,Projection::CharacterProfile))throw std::runtime_error("Final recharge original source differs");
  auto changed=Query("UPDATE inventory SET item_id="+std::to_string(d.output)+",charges="+std::to_string(d.charges)+" WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(device_slot)+" AND item_id="+std::to_string(d.input)+" AND guid="+std::to_string(before.values.guid));if(changed.RowsAffected()>1)throw std::runtime_error("Recharge device update count differs"); // already-full self recharge may change zero rows
  for(const auto& part:parts)if(!part.device){auto removed=Query("DELETE FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(part.slot)+" AND item_id="+std::to_string(part.id)+" AND guid="+std::to_string(part.row.values.guid));if(removed.RowsAffected()!=1)throw std::runtime_error("Recharge exact reagent debit failed");}
  auto post=Query("SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(device_slot)+" FOR UPDATE");if(post.RowCount()!=1)throw std::runtime_error("Recharge postimage missing");auto stored=EQEmu::SharedCore::InventoryRowDB::Decode(post.begin());stored.evolving_payload=Evo::Capture(database,who,device_slot,*prepared);
  if(stored.evolving_payload!=before.evolving_payload||EQEmu::SharedCore::InventoryRowDB::Values(stored,device_slot)!=EQEmu::SharedCore::InventoryRowDB::Values(after,device_slot)||!EQEmu::SharedCore::InventoryProjection::Matches(stored,*prepared,Projection::CharacterProfile))throw std::runtime_error("Recharge exact stored postimage differs");
  auto remaining=Query("SELECT slot_id FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id BETWEEN "+std::to_string(first)+" AND "+std::to_string(first+EQ::invbag::SLOT_COUNT-1)+" ORDER BY slot_id FOR UPDATE");if(remaining.RowCount()!=1||Number(remaining.begin()[0])!=uint32_t(device_slot))throw std::runtime_error("Recharge remaining child universe differs");
  state=Query("SELECT CONNECTION_ID(),@@in_transaction");if(state.RowCount()!=1||Number(state.begin()[0])!=connection||Number(state.begin()[1])!=1||database.TransactionStrictFailed()||!original())throw std::runtime_error("Recharge strict final session differs");
  attempted=true;auto commit=database.TransactionCommitStrict();transaction=false;if(!commit.Success())throw std::runtime_error("Recharge COMMIT unknown");known=true;
  // Prepared packets/clones/proof/retirement capacity already exist. Publish with no callback/allocation.
  auto live=kit->m_contents.find(device_child);live->second=prepared.release();prepared.reset(device);
  for(const auto& part:parts)if(!part.device){auto at=kit->m_contents.find(part.child);spent.emplace_back(at->second);kit->m_contents.erase(at);}
  auto* send=packet.release();try{c.FastQueuePacket(&send);}catch(...){}safe_delete(send);
  return proof;
 }catch(...){if(transaction)database.TransactionRollbackStrict();if(attempted&&!known&&unknown){unknown->Attach();unknown.release();}return nullptr;}
}
bool Custody::Claim(Client& c,const DBTradeskillRecipe_Struct& s)noexcept{const auto* d=Find(s.recipe_id);if(!d||!current_||current_->client_!=&c||current_->character_!=c.CharacterID()||current_->account_!=c.AccountID()||current_->recipe_!=s.recipe_id||current_->claimed_||!Shape(*d,s))return false;current_->claimed_=true;return true;}
bool Custody::Complete(Client& c,std::unique_ptr<Proof> proof,DBTradeskillRecipe_Struct& s){if(!proof||proof->client_!=&c||proof->recipe_!=s.recipe_id||proof->claimed_)return false;auto* previous=current_;current_=proof.get();struct Restore{Proof*& current;Proof* old;~Restore(){current=old;}}restore{current_,previous};return c.TradeskillExecute(&s);}
}
