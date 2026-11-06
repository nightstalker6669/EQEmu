#include "housing_recipe301.h"
#include "client.h"
#include "inventory_session_owner.h"
#include "../common/fire_native_inventory_fence.h"
#include "../common/database.h"
#include "../common/eqemu_config.h"
#include "../common/housing/current_estate_access.h"
#include "../common/housing/custody_codec.h"
#include "../common/housing/item_storage_db.h"
#include "../common/housing/rent_state_db.h"
#include <cstring>
#include <stdexcept>
namespace HousingRecipe301 {
namespace {
auto Q(Database& db,const std::string& sql) {if(db.GetStatus()!=DBcore::Connected)throw std::runtime_error("Station connection unavailable");auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Station authority query failed");return r;}
std::string Hex(const uint8_t* p,size_t size){std::string s;const char* h="0123456789ABCDEF";for(size_t n=0;n<size;++n){s+=h[p[n]>>4];s+=h[p[n]&15];}return s;}
std::string Pose(const Housing::PlacementPolicy::Transform& p){std::vector<uint8_t>b;for(float f:{p.heading,p.pitch,p.roll,p.scale,p.position[0],p.position[1],p.position[2]}){uint32_t n;std::memcpy(&n,&f,4);for(unsigned j=0;j<4;++j)b.push_back(uint8_t(n>>(j*8)));}return Hex(b.data(),b.size());}
}
Use::~Use() noexcept {try{if(db_&&db_->GetStatus()==DBcore::Connected)db_->QueryDatabase(std::string("ROLLBACK"),false);}catch(...){}}
bool Use::Held() noexcept {try{if(!db_)return false;auto r=Q(*db_,"SELECT CONNECTION_ID(),@@in_transaction");return r.RowCount()==1&&r.begin()[0]&&r.begin()[1]&&std::stoull(r.begin()[0])==connection_&&std::stoul(r.begin()[1])==1;}catch(...){return false;}}
// Source authority linearizes at this one admission, while its locks are held.
// After the first inventory debit the ordinary admitted combine must finish;
// loss of this READ connection cannot create a new partial-debit abort.
bool Use::AdmitDebit() noexcept {
 if(debit_admitted_ || !Held())return false;
 debit_admitted_=true;
 return true;
}

std::unique_ptr<Use> Use::Acquire(Client& c,const Binding& b) noexcept {
 try {
  if(!FirePersonalOccurrenceSource::NativeInventoryFence::ProfileAllowed(&c.GetInv())||!c.ValidateHousingRecipe301(b)||!b.stored.placement||b.stored.row.values.item_id!=54250)return nullptr;
  auto ticket=b.owner;auto lease=ticket?ticket->SnapshotFor(c):std::nullopt;
  if(!ticket||!lease||!lease->active||!ticket->HeldFor(c))return nullptr;
  const auto* config=EQEmuConfig::get();if(!config)return nullptr;
  auto out=std::unique_ptr<Use>(new Use);out->db_=std::make_unique<Database>();auto& db=*out->db_;
  if(!db.Connect(config->DatabaseHost,config->DatabaseUsername,config->DatabasePassword,config->DatabaseDB,config->DatabasePort,"recipe301-station-authority"))return nullptr;
  Q(db,"SET SESSION innodb_lock_wait_timeout=2");Q(db,"START TRANSACTION");auto state=Q(db,"SELECT CONNECTION_ID(),@@in_transaction");if(state.RowCount()!=1||!state.begin()[0]||!state.begin()[1]||std::stoul(state.begin()[1])!=1)return nullptr;out->connection_=std::stoull(state.begin()[0]);
  const auto& l=*lease;
  auto actor=Q(db,"SELECT account_id,epoch,HEX(token),process_key,client_key,zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(c.CharacterID())+" FOR UPDATE");
  if(actor.RowCount()!=1)return nullptr;auto ar=actor.begin();for(unsigned n=0;n<8;++n)if(!ar[n])return nullptr;
  if(std::stoul(ar[0])!=c.AccountID()||std::stoull(ar[1])!=l.epoch||std::string(ar[2])!=Hex(l.token.data(),l.token.size())||std::string(ar[3])!=l.owner.process_key||std::string(ar[4])!=l.owner.client_key||std::stoul(ar[5])!=b.zone||std::stoul(ar[6])!=b.instance||std::stoul(ar[7])!=1)return nullptr;
  const auto& p=*b.stored.placement;
  // Existing custody/pose writers take these same rows FOR UPDATE. They cannot
  // move, transfer, pick up, stow, or replace this original table during debit.
  auto custody=Q(db,"SELECT estate_id,owner_character_id,HEX(row_blob) FROM housing_live_item_custody WHERE custody_id="+std::to_string(b.stored.custody_id)+" FOR UPDATE");
  auto raw=Housing::CustodyCodec::Encode(b.stored.row);
  if(custody.RowCount()!=1||!custody.begin()[0]||!custody.begin()[1]||!custody.begin()[2]||std::stoul(custody.begin()[0])!=b.stored.estate_id||std::stoul(custody.begin()[1])!=b.stored.row.values.character_id||std::string(custody.begin()[2])!=Hex(raw.data(),raw.size()))return nullptr;
  auto pose=Q(db,"SELECT native_item_id,pose_revision,HEX(pose_blob) FROM housing_live_placed_items WHERE custody_id="+std::to_string(b.stored.custody_id)+" FOR UPDATE");
  if(pose.RowCount()!=1||!pose.begin()[0]||!pose.begin()[1]||!pose.begin()[2]||std::stoul(pose.begin()[0])!=p.reservation.native_item_id||std::stoull(pose.begin()[1])!=p.pose_revision||std::string(pose.begin()[2])!=Pose(p.transform))return nullptr;
  auto reservation=Q(db,"SELECT state,custody_id,native_estate_id,estate_id,catalog_key FROM housing_live_placement_reservations WHERE native_item_id="+std::to_string(p.reservation.native_item_id)+" FOR UPDATE");
  if(reservation.RowCount()!=1)return nullptr;auto rr=reservation.begin();for(unsigned n=0;n<5;++n)if(!rr[n])return nullptr;
  if(std::stoul(rr[0])!=1||std::stoull(rr[1])!=b.stored.custody_id||std::stoul(rr[2])!=b.native_plot||std::stoul(rr[3])!=b.stored.estate_id||std::string(rr[4])!=p.reservation.catalog_key)return nullptr;
  auto storage=Housing::ItemStorage::Locked(db,b.stored.custody_id);if(storage.stowed||storage.revision!=b.stored.storage_revision||Housing::HouseItemScope::Locked(db,b.stored.custody_id))return nullptr;
  auto catalog=Q(db,"SELECT revision FROM housing_catalogs WHERE catalog_key='"+db.Escape(p.reservation.catalog_key)+"' FOR UPDATE");if(catalog.RowCount()!=1||!catalog.begin()[0]||std::stoull(catalog.begin()[0])!=b.catalog_revision)return nullptr;
  auto mapping=Q(db,"SELECT h.zone_id,h.zone_version,h.instance_id FROM housing_native_estates n JOIN housing_neighborhoods h ON h.id=n.neighborhood_id WHERE n.native_id="+std::to_string(b.native_plot)+" AND n.ownership_estate_id="+std::to_string(b.stored.estate_id)+" AND n.active=1 FOR UPDATE");
  if(mapping.RowCount()!=1||!mapping.begin()[0]||!mapping.begin()[1]||!mapping.begin()[2]||std::stoul(mapping.begin()[0])!=b.zone||std::stoul(mapping.begin()[1])!=b.version||std::stoul(mapping.begin()[2])!=b.instance)return nullptr;
  auto access=Housing::LoadCurrentEstateAccessUnderTransaction(db,b.stored.estate_id,b.native_plot,{c.CharacterID(),c.AccountID()});
  if(!access||access.value.acl_revision!=b.acl_revision||!access.value.effective.Allows(Housing::AccessPolicy::Capability::InteractItem)||!Housing::RentState::Plot(db,b.stored.estate_id,b.native_plot).Active())return nullptr;
  if(!ticket->HeldFor(c)||!out->Held())return nullptr;
  return out;
 }catch(...){return nullptr;}
}
}
