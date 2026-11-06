#include "trophy_certificate55.h"
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
#include <cstring>
#include <stdexcept>
extern ZoneDatabase database;extern Zone* zone;
namespace TrophyCertificate55 {
thread_local Proof* Custody::current_=nullptr;
namespace {
using Row=EQEmu::SharedCore::InventoryRow;
using Projection=EQEmu::SharedCore::InventoryProjection::ReconstructionScope;
namespace Evo=Housing::EvolvingItemCustody;
struct WorldUnknown {uint32_t zone=0,parent=0;WorldUnknown* next=nullptr;inline static thread_local WorldUnknown* first=nullptr;void Attach()noexcept{next=first;first=this;}};
uint64_t Number(const char* p){if(!p)throw std::runtime_error("Missing trophy identity");size_t n=0;std::string s=p;auto v=std::stoull(s,&n);if(n!=s.size())throw std::runtime_error("Malformed trophy identity");return v;}
auto Query(const std::string& sql){if(database.GetStatus()!=DBcore::Connected)throw std::runtime_error("Trophy database disconnected");auto r=database.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Strict trophy query failed");return r;}
std::string Hex(const void* data,size_t size){const char* h="0123456789ABCDEF";const auto* p=static_cast<const uint8_t*>(data);std::string s;s.reserve(size*2);for(size_t i=0;i<size;++i){s+=h[p[i]>>4];s+=h[p[i]&15];}return s;}
std::string Hex(const std::string& s){return Hex(s.data(),s.size());}
bool Shape(const Definition& d,const DBTradeskillRecipe_Struct& s){return s.recipe_id==d.recipe&&uint32_t(s.tradeskill)==d.skill&&s.nofail&&s.trivial==0&&s.skill_needed==0&&!s.quest&&!s.replace_container&&!s.must_learn&&s.onsuccess.size()==1&&s.onsuccess.begin()->first==d.output&&s.onsuccess.begin()->second==1;}
bool Actor(Client& c,const Definition& d){if(d.skill==59)return c.GetClass()==Class::Shaman&&c.GetLevel()>=25;if(d.skill==57)return c.GetPP().race==Race::Gnome;if(d.skill==56)return c.GetClass()==Class::Rogue;return true;}
}
bool Custody::WorldAllowed(uint32_t z,uint32_t id)noexcept{for(auto* p=WorldUnknown::first;p;p=p->next)if(p->zone==z&&p->parent==id)return false;return true;}
std::unique_ptr<Proof> Custody::Manual(Client& c,DBTradeskillRecipe_Struct& s,Object* world,EQ::ItemInstance* container,int16_t slot)noexcept {
 const auto* d=Find(s.recipe_id);if(!d||!container||container->GetContents()->size()!=1)return nullptr;
 const auto& entry=*container->GetContents()->begin();if(!entry.second||entry.second->GetID()!=d->certificate||entry.first>=EQ::invtype::WORLD_SIZE)return nullptr;
 return Execute(c,*d,s,world,container,slot,entry.first,true);
}
std::unique_ptr<Proof> Custody::Auto(Client& c,DBTradeskillRecipe_Struct& s,uint32_t type,uint32_t kit)noexcept {
 const auto* d=Find(s.recipe_id);if(!d||type!=d->type||(d->kit>100&&kit!=d->kit))return nullptr;
 EQ::ItemInstance* container=nullptr;Object* world=nullptr;
 if(d->kit<100){world=c.GetTradeskillObject();if(!world)return nullptr;container=world->m_inst;}
 else {const auto slot=c.GetInv().HasItem(d->kit,1,invWherePersonal);container=c.GetInv().GetItem(slot);if(!container)return nullptr;}
 return Execute(c,*d,s,world,container,-1,0,false);
}
std::unique_ptr<Proof> Custody::Execute(Client& c,const Definition& d,DBTradeskillRecipe_Struct& spec,Object* world,EQ::ItemInstance* kit,int16_t parent_slot,uint8_t child,bool manual)noexcept {
 bool transaction=false,attempted=false,known=false;std::unique_ptr<FungalInventory::UnknownInventory> unknown;std::unique_ptr<WorldUnknown> world_unknown;
 auto& inv=c.GetInv();
 try {
  if(!zone||!Shape(d,spec)||!Actor(c,d)||!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&inv))return nullptr;
  const auto who=c.CharacterID(),account=c.AccountID(),z=zone->GetZoneID();
  const auto version=zone->GetInstanceVersion();
  auto reservation=c.InventorySessionReleaseTicket();auto lease=reservation?reservation->SnapshotFor(c):std::nullopt;
  if(!who||!account||!lease||!lease->active||!c.HasInventorySessionReservation()||lease->actor.character_id!=who||lease->actor.account_id!=account)return nullptr;
  const uint32_t world_id=world?world->GetDBID():0;
  if(d.kit<100){if(!world||world!=c.GetTradeskillObject()||world->user!=&c||world->m_inst!=kit||world->GetType()!=d.type||!world_id||world->GetItemID()!=0||!kit->IsType(EQ::item::ItemClassBag)||!WorldAllowed(z,world_id))return nullptr;}
  else {if(world||!kit||!kit->IsClassBag()||kit->GetID()!=d.kit||kit->GetItem()->BagType!=d.type||kit->GetItem()->BagSlots!=d.slots||kit->GetItem()->BagSize!=d.size)return nullptr;bool owned=false;for(int16_t i=EQ::invslot::GENERAL_BEGIN;i<=EQ::invslot::GENERAL_END;++i)if(inv.GetLookup()&&((uint64_t(1)<<i)&inv.GetLookup()->PossessionsBitmask)&&inv.GetItem(i)==kit){if(manual&&i!=parent_slot)return nullptr;owned=true;parent_slot=i;break;}if(!owned)return nullptr;}
  EQ::ItemInstance** trophy_live=nullptr;int16_t trophy_slot=-1;
  for(int16_t slot:{int16_t(EQ::invslot::slotPrimary),int16_t(EQ::invslot::slotSecondary)}){auto at=inv.m_worn.find(slot);if(at!=inv.m_worn.end()&&at->second&&at->second->GetID()==d.beginner){if(trophy_live)return nullptr;trophy_live=&at->second;trophy_slot=slot;}}
  if(!trophy_live)return nullptr;auto* trophy=*trophy_live;const auto* old=trophy->GetItem();const auto* target=database.GetItem(d.output);
  if(!old||!target||!trophy->IsClassCommon()||trophy->IsScaling()||trophy->m_scaledItem||!trophy->IsEvolving()||trophy->GetEvolveUniqueID()==0||trophy->GetSerialNumber()<=0||old->EvolvingID!=d.family||old->EvolvingLevel!=1||target->ItemClass!=0||!target->EvolvingItem||target->EvolvingID!=d.family||target->EvolvingLevel!=d.level||target->EvolvingMax!=old->EvolvingMax||old->CharmFileID||target->CharmFileID||!(target->Slots&(uint32_t(1)<<trophy_slot)))return nullptr;
  for(unsigned i=0;i<6;++i)if(old->AugSlotType[i]!=target->AugSlotType[i])return nullptr;
  EQ::ItemInstance** certificate_live=nullptr;EQ::ItemInstance* certificate=nullptr;int16_t certificate_slot=-1;
  if(manual){auto at=kit->m_contents.find(child);if(kit->m_contents.size()!=1||at==kit->m_contents.end()||!at->second||at->second->GetID()!=d.certificate)return nullptr;certificate_live=&at->second;certificate=at->second;if(!world)certificate_slot=EQ::InventoryProfile::CalcSlotId(parent_slot,child);}
  else {for(int16_t slot=EQ::invslot::GENERAL_BEGIN;slot<=EQ::invslot::GENERAL_END&&!certificate;++slot){if(!inv.GetLookup()||!((uint64_t(1)<<slot)&inv.GetLookup()->PossessionsBitmask))continue;auto at=inv.m_inv.find(slot);if(at==inv.m_inv.end()||!at->second)continue;if(at->second->GetID()==d.certificate){certificate_live=&at->second;certificate=at->second;certificate_slot=slot;break;}if(at->second->IsClassBag())for(auto& x:at->second->m_contents)if(x.second&&x.second->GetID()==d.certificate){certificate_live=&x.second;certificate=x.second;certificate_slot=EQ::InventoryProfile::CalcSlotId(slot,x.first);break;}}}
  if(!certificate||!certificate_live||(!world&&certificate->GetItem()&&certificate->GetItem()->Size>d.size)||!certificate->IsClassCommon()||certificate->IsStackable()||certificate->GetSerialNumber()<=0||!certificate->GetContents()->empty()||certificate->IsEvolving())return nullptr;
  const auto serial=trophy->GetSerialNumber(),certificate_serial=certificate->GetSerialNumber();
  const auto original=[&](){return FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&inv)&&reservation->HeldFor(c)&&*trophy_live==trophy&&*certificate_live==certificate&&trophy->GetID()==d.beginner&&trophy->GetSerialNumber()==serial&&certificate->GetID()==d.certificate&&certificate->GetSerialNumber()==certificate_serial&&Actor(c,d)&&(!world||(world==c.GetTradeskillObject()&&world->user==&c&&world->m_inst==kit&&world->GetDBID()==world_id&&WorldAllowed(z,world_id)));};
  if(!original())return nullptr;
  // Clone retains every owned augment, native GUID, timers and custom fields.
  std::unique_ptr<EQ::ItemInstance> prepared(trophy->Clone());if(!prepared)return nullptr;
  const auto copy_delivered=[&](const auto& self,const EQ::ItemInstance& a,EQ::ItemInstance& b)->void{b.m_task_delivered_count=a.m_task_delivered_count;for(const auto& x:a.m_contents){auto at=b.m_contents.find(x.first);if(!x.second||at==b.m_contents.end()||!at->second)throw std::runtime_error("Trophy clone tree differs");self(self,*x.second,*at->second);}};copy_delivered(copy_delivered,*trophy,*prepared);
  auto definition=std::make_unique<EQ::ItemData>(*target);delete prepared->m_item;prepared->m_item=definition.release();
  auto detail=prepared->GetEvolvingDetails();detail.item_id=d.output;detail.current_amount=0;detail.progression=0;if(!prepared->SetEvolvingDetails(detail))return nullptr;
  auto proof=std::unique_ptr<Proof>(new Proof(c,d.recipe));
  unknown=std::unique_ptr<FungalInventory::UnknownInventory>(new FungalInventory::UnknownInventory);unknown->character_=who;unknown->inventory_=&inv;
  const auto remember=[&](const auto& self,const EQ::ItemInstance* item)->void{if(!item)return;unknown->items_.push_back(item);for(const auto& x:*item->GetContents())self(self,x.second);};
  for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& x:*bucket)remember(remember,x.second);for(auto at=inv.cursor_cbegin();at!=inv.cursor_cend();++at)remember(remember,*at);
  if(world&&manual){remember(remember,kit);world_unknown=std::make_unique<WorldUnknown>();world_unknown->zone=z;world_unknown->parent=world_id;}
  auto state=Query("SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");if(state.RowCount()!=1||Number(state.begin()[1])||Number(state.begin()[2])!=1)return nullptr;const auto connection=Number(state.begin()[0]);if(!database.TransactionBeginStrict().Success())return nullptr;transaction=true;
  const std::string tables="'inventory','character_data','housing_inventory_sessions','housing_live_item_leases','character_evolving_items','housing_evolving_item_locations','tradeskill_recipe','tradeskill_recipe_entries','items','items_evolving_details'"+(world&&manual?std::string(",'object','object_contents'"):std::string());
  auto engines=Query("SELECT COUNT(*)FROM information_schema.tables WHERE table_schema=DATABASE()AND engine='InnoDB'AND table_name IN("+tables+")");if(engines.RowCount()!=1||Number(engines.begin()[0])!=(world&&manual?12u:10u))throw std::runtime_error("Trophy strict foundation differs");
  auto actor=Query("SELECT id FROM character_data WHERE id="+std::to_string(who)+" AND account_id="+std::to_string(account)+" AND deleted_at IS NULL FOR UPDATE");if(actor.RowCount()!=1)throw std::runtime_error("Trophy actor changed");
  auto held=Query("SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(who)+" FOR UPDATE");if(held.RowCount()!=1)throw std::runtime_error("Trophy lease absent");auto h=held.begin();
  if(Number(h[0])!=account||Number(h[1])!=lease->epoch||!h[2]||std::string(h[2])!=Hex(lease->token.data(),lease->token.size())||!h[3]||std::string(h[3])!=Hex(lease->owner.process_key)||!h[4]||std::string(h[4])!=Hex(lease->owner.client_key)||Number(h[5])!=lease->owner.zone_id||Number(h[6])!=lease->owner.instance_id||Number(h[7])!=1)throw std::runtime_error("Trophy lease changed");
  auto pending=Query("SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+std::to_string(who)+" FOR UPDATE");if(pending.RowCount()>1||(pending.RowCount()&&(Number(pending.begin()[0])!=account||(pending.begin()[1]&&*pending.begin()[1]))))throw std::runtime_error("Trophy pending custody");
  auto recipe=Query("SELECT id FROM tradeskill_recipe WHERE id="+std::to_string(d.recipe)+" AND tradeskill="+std::to_string(d.skill)+" AND nofail=1 AND trivial=0 AND skillneeded=0 AND quest=0 AND replace_container=0 AND must_learn=0 AND learned_by_item_id=0 AND enabled=1 FOR UPDATE");if(recipe.RowCount()!=1)throw std::runtime_error("Trophy recipe changed");
  auto roles=Query("SELECT item_id,successcount,failcount,componentcount,iscontainer FROM tradeskill_recipe_entries WHERE recipe_id="+std::to_string(d.recipe)+" ORDER BY item_id FOR UPDATE");bool input=false,output=false,container=false;if(roles.RowCount()!=3)throw std::runtime_error("Trophy recipe shape differs");for(auto r=roles.begin();r!=roles.end();++r){if(Number(r[0])==d.certificate&&Number(r[1])==0&&Number(r[2])==0&&Number(r[3])==1&&Number(r[4])==0)input=true;else if(Number(r[0])==d.output&&Number(r[1])==1&&Number(r[2])==0&&Number(r[3])==0&&Number(r[4])==0)output=true;else if(Number(r[0])==d.kit&&Number(r[1])==0&&Number(r[2])==0&&Number(r[3])==0&&Number(r[4])==1)container=true;else throw std::runtime_error("Trophy unexpected recipe role");}if(!input||!output||!container)throw std::runtime_error("Trophy exact roles absent");
  auto evolution=Query("SELECT item_id,item_evo_id,item_evolve_level,type FROM items_evolving_details WHERE item_id IN("+std::to_string(d.beginner)+","+std::to_string(d.output)+") ORDER BY item_id FOR UPDATE");if(evolution.RowCount()!=2)throw std::runtime_error("Trophy evolution definition absent");for(auto r=evolution.begin();r!=evolution.end();++r)if(Number(r[1])!=d.family||Number(r[2])!=(Number(r[0])==d.beginner?1u:d.level)||Number(r[3])!=99)throw std::runtime_error("Trophy evolution family changed");
  auto worn=Query("SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(trophy_slot)+" FOR UPDATE");if(worn.RowCount()!=1)throw std::runtime_error("Original worn trophy absent");auto before=EQEmu::SharedCore::InventoryRowDB::Decode(worn.begin());before.evolving_payload=Evo::Capture(database,who,trophy_slot,*trophy);if(before.values.guid!=uint64_t(serial)||!EQEmu::SharedCore::InventoryProjection::Matches(before,*trophy,Projection::CharacterProfile))throw std::runtime_error("Original trophy full property differs");
  // Capture validates owner/item/slot/component. Fence the ORIGINAL root
  // GUID for every existing root or evolving-augment locator as well.
  // An absent locator retains Capture's separately checked unique-legacy path.
  for(const auto& component:Evo::Decode(before.evolving_payload)){
   auto locator=Query("SELECT inventory_guid FROM housing_evolving_item_locations WHERE evolving_id="+std::to_string(component.id)+" FOR UPDATE");
   if(locator.RowCount()>1||(locator.RowCount()==1&&Number(locator.begin()[0])!=before.values.guid))throw std::runtime_error("Original trophy evolving locator GUID differs");
  }
  auto after=before;after.values.item_id=d.output;auto states=Evo::Decode(after.evolving_payload);bool root=false;for(auto& s:states)if(!s.component){if(s.id!=trophy->GetEvolveUniqueID()||s.item!=d.beginner)throw std::runtime_error("Trophy evolving UID differs");s.item=d.output;s.amount=0;s.progression_bits=s.runtime_progression_bits=0;s.nulls&=uint8_t(~(16u|32u));root=true;}if(!root)throw std::runtime_error("Trophy evolving identity absent");after.evolving_payload=Evo::Encode(states);prepared->m_color=EQEmu::SharedCore::InventoryProjection::Color(after,*target);if(!EQEmu::SharedCore::InventoryProjection::Matches(after,*prepared,Projection::CharacterProfile))throw std::runtime_error("Trophy transform loses property");
  const auto conflict=[&](const auto& self,const EQ::ItemInstance* item)->bool{if(!item)return false;const auto* data=item->GetItem();if(item!=trophy&&data&&target->LoreGroup&&((target->LoreGroup==-1&&item->GetID()==d.output)||(target->LoreGroup!=-1&&data->LoreGroup==target->LoreGroup)))return true;for(const auto& x:*item->GetContents())if(self(self,x.second))return true;return false;};
  for(const auto* bucket:{&inv.m_worn,&inv.m_inv,&inv.m_bank,&inv.m_shbank,&inv.m_trade})for(const auto& x:*bucket)if(conflict(conflict,x.second))throw std::runtime_error("Trophy target lore conflict");for(auto at=inv.cursor_cbegin();at!=inv.cursor_cend();++at)if(conflict(conflict,*at))throw std::runtime_error("Trophy cursor lore conflict");
  if(world){auto object=Query("SELECT id FROM object WHERE id="+std::to_string(world_id)+" AND zoneid="+std::to_string(z)+" AND version="+std::to_string(version)+" AND type="+std::to_string(d.type)+" AND itemid=0 FOR UPDATE");if(object.RowCount()!=1)throw std::runtime_error("Original trophy station changed");}
  if(world&&manual){
   // The existing object_contents key has no instance dimension. Never claim
   // an instanced World certificate preimage using that shared legacy key.
   if(zone->GetInstanceID()!=0)throw std::runtime_error("Unsupported instanced World certificate custody");
   auto rows=Query("SELECT bagidx,itemid,charges,augslot1,augslot2,augslot3,augslot4,augslot5,augslot6 FROM object_contents WHERE zoneid="+std::to_string(z)+" AND parentid="+std::to_string(world_id)+" ORDER BY bagidx FOR UPDATE");if(rows.RowCount()!=1)throw std::runtime_error("Original World certificate universe changed");auto r=rows.begin();if(Number(r[0])!=child||Number(r[1])!=d.certificate||!r[2]||std::stoi(r[2])!=certificate->GetCharges())throw std::runtime_error("Original World certificate differs");for(int i=3;i<9;++i)if(Number(r[i]))throw std::runtime_error("Augmented certificate unsupported");
  }else{
   auto rows=Query("SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(certificate_slot)+" FOR UPDATE");if(rows.RowCount()!=1)throw std::runtime_error("Original inventory certificate absent");auto row=EQEmu::SharedCore::InventoryRowDB::Decode(rows.begin());if(row.values.guid!=uint64_t(certificate_serial)||!EQEmu::SharedCore::InventoryProjection::Matches(row,*certificate,Projection::CharacterProfile))throw std::runtime_error("Original certificate full property differs");
   if(manual){auto parents=Query("SELECT "+std::string(EQEmu::SharedCore::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(parent_slot)+" FOR UPDATE");if(parents.RowCount()!=1)throw std::runtime_error("Original trophy kit absent");auto parent=EQEmu::SharedCore::InventoryRowDB::Decode(parents.begin());row.values.slot_id=child;parent.children.push_back(row);if(!EQEmu::SharedCore::InventoryProjection::Matches(parent,*kit,Projection::CharacterProfile))throw std::runtime_error("Original trophy kit full property differs");auto children=Query("SELECT slot_id FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id BETWEEN "+std::to_string(EQ::InventoryProfile::CalcSlotId(parent_slot,uint8_t(0)))+" AND "+std::to_string(EQ::InventoryProfile::CalcSlotId(parent_slot,uint8_t(0))+EQ::invbag::SLOT_COUNT-1)+" FOR UPDATE");if(children.RowCount()!=1)throw std::runtime_error("Original trophy kit child universe changed");}
  }
  std::vector<std::unique_ptr<EQApplicationPacket>> packets;
  {const auto bytes=prepared->Serialize(trophy_slot);auto p=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));auto* w=reinterpret_cast<ItemPacket_Struct*>(p->pBuffer);w->PacketType=ItemPacketTrade;std::memcpy(w->SerializedItem,bytes.data(),bytes.size());packets.push_back(std::move(p));}
  if(world&&manual){auto p=std::make_unique<EQApplicationPacket>(OP_ClearObject,sizeof(ClearObject_Struct));reinterpret_cast<ClearObject_Struct*>(p->pBuffer)->Clear=1;packets.push_back(std::move(p));}
  else {auto p=std::make_unique<EQApplicationPacket>(OP_MoveItem,sizeof(MoveItem_Struct));auto* w=reinterpret_cast<MoveItem_Struct*>(p->pBuffer);w->from_slot=certificate_slot;w->to_slot=0xffffffff;w->number_in_stack=0xffffffff;packets.push_back(std::move(p));}
  if(!original())throw std::runtime_error("Final trophy source fence differs");
  auto evolved=Query("UPDATE character_evolving_items SET item_id="+std::to_string(d.output)+",current_amount=0,progression=0 WHERE id="+std::to_string(trophy->GetEvolveUniqueID())+" AND character_id="+std::to_string(who)+" AND item_id="+std::to_string(d.beginner)+" AND deleted_at IS NULL");if(evolved.RowsAffected()!=1)throw std::runtime_error("Exact trophy evolution update failed");
  auto replaced=Query("UPDATE inventory SET item_id="+std::to_string(d.output)+" WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(trophy_slot)+" AND item_id="+std::to_string(d.beginner)+" AND guid="+std::to_string(serial));if(replaced.RowsAffected()!=1)throw std::runtime_error("Exact worn trophy update failed");
  Evo::BindInventory(database,who,trophy_slot,*prepared);
  auto removed=world&&manual?Query("DELETE FROM object_contents WHERE zoneid="+std::to_string(z)+" AND parentid="+std::to_string(world_id)+" AND bagidx="+std::to_string(child)+" AND itemid="+std::to_string(d.certificate)):Query("DELETE FROM inventory WHERE character_id="+std::to_string(who)+" AND slot_id="+std::to_string(certificate_slot)+" AND item_id="+std::to_string(d.certificate)+" AND guid="+std::to_string(certificate_serial));if(removed.RowsAffected()!=1)throw std::runtime_error("Exact trophy certificate debit failed");
  state=Query("SELECT CONNECTION_ID(),@@in_transaction");if(state.RowCount()!=1||Number(state.begin()[0])!=connection||Number(state.begin()[1])!=1||database.TransactionStrictFailed()||!original())throw std::runtime_error("Trophy strict final session differs");
  attempted=true;auto commit=database.TransactionCommitStrict();transaction=false;if(!commit.Success())throw std::runtime_error("Trophy COMMIT unknown");known=true;
  // No allocation or mutable callback before the original native publication.
  *trophy_live=prepared.release();prepared.reset(trophy);
  if(manual){kit->m_contents.erase(child);}
  else {bool erased=false;auto at=inv.m_inv.find(certificate_slot);if(at!=inv.m_inv.end() && &at->second==certificate_live){inv.m_inv.erase(at);erased=true;}if(!erased)for(auto& parent:inv.m_inv)if(parent.second&&parent.second->IsClassBag()){for(auto at=parent.second->m_contents.begin();at!=parent.second->m_contents.end();++at)if(&at->second==certificate_live){parent.second->m_contents.erase(at);erased=true;break;}if(erased)break;}}
  std::unique_ptr<EQ::ItemInstance> spent(certificate);
  for(auto& p:packets){auto* send=p.release();try{c.FastQueuePacket(&send);}catch(...){}safe_delete(send);}
  return proof;
 }catch(...){if(transaction)database.TransactionRollbackStrict();if(attempted&&!known&&unknown){unknown->Attach();unknown.release();if(world_unknown){world_unknown->Attach();world_unknown.release();}}return nullptr;}
}
bool Custody::Claim(Client& c,const DBTradeskillRecipe_Struct& s)noexcept{const auto* d=Find(s.recipe_id);if(!d)return false;if(!current_||current_->client_!=&c||current_->recipe_!=s.recipe_id||current_->claimed_||!Shape(*d,s))return false;current_->claimed_=true;return true;}
bool Custody::Complete(Client& c,std::unique_ptr<Proof> proof,DBTradeskillRecipe_Struct& s){if(!proof||proof->client_!=&c||proof->recipe_!=s.recipe_id||proof->claimed_)return false;auto* previous=current_;current_=proof.get();struct Restore{Proof*& current;Proof* old;~Restore(){current=old;}}restore{current_,previous};return c.TradeskillExecute(&s);}
}
