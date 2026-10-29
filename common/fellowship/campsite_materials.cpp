#include "campsite_materials.h"
#include "../database.h"
#include "../housing/inventory_row_db.h"
#include "../housing/coin_pending_guard.h"
#include "../housing/house_inventory_transition.h"
#include <stdexcept>
namespace Fellowship { namespace Campsites {
namespace {
auto Q(Database& db,const std::string& sql) { if(db.GetStatus()!=DBcore::Connected)throw std::runtime_error("Campsite material connection lost"); auto result=db.QueryDatabase(sql,false);if(!result.Success())throw std::runtime_error("Campsite material query failed");return result; }
uint64_t N(const char* s) { if(!s)throw std::runtime_error("Campsite missing row field");return std::stoull(s); }
std::string Hex(const uint8_t* data,size_t size) {const char* digits="0123456789ABCDEF";std::string out;for(size_t i=0;i<size;++i){out+=digits[data[i]>>4];out+=digits[data[i]&15];}return out;}
std::string Hex(const std::string& s){return Hex(reinterpret_cast<const uint8_t*>(s.data()),s.size());}
}
bool ConsumeMaterialsUnderTransaction(Database& db,uint32_t id,const Housing::InventorySessions::Lease& lease,const std::array<Housing::LiveItems::InventoryRow,2>& expected) {
 try {
  const auto* definition=Lookup(id);
  if(!definition || !definition->implemented || !definition->kit_item_id || !definition->lumber_item_id || !lease.active || !lease.actor.character_id || !lease.actor.account_id || !lease.epoch || lease.owner.process_key.empty() || lease.owner.client_key.empty())return false;
  auto tx=Q(db,"SELECT @@in_transaction");if(tx.RowCount()!=1 || N(tx.begin()[0])!=1)return false;
  auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','housing_inventory_sessions','fellowships')");if(engines.RowCount()!=1 || N(engines.begin()[0])!=3)return false;
  const auto actor=std::to_string(lease.actor.character_id);
  auto character=Q(db,"SELECT id FROM character_data WHERE id="+actor+" AND account_id="+std::to_string(lease.actor.account_id)+" AND deleted_at IS NULL FOR UPDATE");if(character.RowCount()!=1)return false;
  auto session=Q(db,"SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+actor+" FOR UPDATE");
  if(session.RowCount()!=1)return false;auto s=session.begin();
  if(N(s[0])!=lease.actor.account_id || N(s[1])!=lease.epoch || !s[2] || std::string(s[2])!=Hex(lease.token.data(),lease.token.size()) || !s[3] || std::string(s[3])!=Hex(lease.owner.process_key) || !s[4] || std::string(s[4])!=Hex(lease.owner.client_key) || N(s[5])!=lease.owner.zone_id || N(s[6])!=lease.owner.instance_id || N(s[7])!=1)return false;
  auto pending=Q(db,"SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+actor+" FOR UPDATE");
  if(pending.RowCount()>1 || (pending.RowCount() && (N(pending.begin()[0])!=lease.actor.account_id || (pending.begin()[1] && *pending.begin()[1]))))return false;
  if(Housing::CoinPending::Inspect(db,lease.actor.character_id,true)!=Housing::CoinPending::State::Clear)return false;
  if(Housing::HouseInventory::InspectPending(db,lease.actor.character_id,true)!=Housing::HouseInventory::PendingState::Clear)return false;
  const auto recipe=RequiredMaterials(*definition);
  if(expected[0].values.slot_id==expected[1].values.slot_id)return false;
  // Validate every component before changing either one, including exact raw
  // identity/null/custom/GUID fields; never infer identity from item ID alone.
  for(size_t i=0;i<expected.size();++i) {
   const auto& e=expected[i];const auto& v=e.values;
   if(v.character_id!=lease.actor.character_id || v.item_id!=recipe[i].item_id || !v.charges || (e.null_fields&(Housing::LiveItems::NullCharges|Housing::LiveItems::NullItemId)) || !e.children.empty() || !e.evolving_payload.empty() || v.augment_one || v.augment_two || v.augment_three || v.augment_four || v.augment_five || v.augment_six)return false;
   auto rows=Q(db,"SELECT "+std::string(Housing::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+actor+" AND slot_id="+std::to_string(v.slot_id)+" FOR UPDATE");
   if(rows.RowCount()!=1 || Housing::CustodyCodec::Encode(Housing::InventoryRowDB::Decode(rows.begin()))!=Housing::CustodyCodec::Encode(e))return false;
   auto item=Q(db,"SELECT itemtype,itemclass,stackable,charmfileid,evoitem FROM items WHERE id="+std::to_string(v.item_id)+" FOR UPDATE");
   if(item.RowCount()!=1 || N(item.begin()[0])!=58 || N(item.begin()[1])!=0 || N(item.begin()[2])!=1 || N(item.begin()[3]) || N(item.begin()[4]))return false;
   if(Q(db,"SELECT id FROM items_evolving_details WHERE item_id="+std::to_string(v.item_id)+" FOR UPDATE").RowCount())return false;
  }
  for(const auto& e:expected) {
   const auto& v=e.values;const auto where="character_id="+actor+" AND slot_id="+std::to_string(v.slot_id)+" AND item_id="+std::to_string(v.item_id)+" AND charges="+std::to_string(v.charges);
   auto debit=Q(db,v.charges==1?"DELETE FROM inventory WHERE "+where:"UPDATE inventory SET charges=charges-1 WHERE "+where);
   if(debit.RowsAffected()!=1)throw std::runtime_error("Campsite material debit changed");
  }
  return true;
 }catch(...){throw;}
}
} }
