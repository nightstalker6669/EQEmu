#include "fellowship_campsite_materials.h"
#include "client.h"
#include "inventory_session_owner.h"
#include "../common/eqemu_config.h"
#include "../common/fellowship/campsite_materials.h"
#include "../common/housing/inventory_projection.h"
#include "../common/housing/inventory_row_db.h"
namespace Camps = Fellowship::Campsites;
namespace {
struct MaterialDebit { int16_t slot{}; EQ::ItemInstance* item{}; int32_t serial{}; int16_t charges{}; uint32_t id{}; };
bool Stage(Client& client,const Camps::Definition& definition,std::array<MaterialDebit,2>& plan) {
 const auto recipe=Camps::RequiredMaterials(definition);
 for(size_t i=0;i<plan.size();++i) {
  const auto slot=client.GetInv().HasItem(recipe[i].item_id,1,invWherePersonal);
  auto* item=client.GetInv().GetItem(slot);
  if(slot==INVALID_INDEX || !item || !item->GetItem() || item->GetItem()->ID!=recipe[i].item_id || item->GetItem()->ItemType!=58 || !item->IsStackable() || item->GetCharges()<1 || item->GetEvolveUniqueID())return false;
  plan[i]={slot,item,item->GetSerialNumber(),item->GetCharges(),recipe[i].item_id};
 }
 return plan[0].slot!=plan[1].slot;
}
bool Frame(Client& client,const std::array<MaterialDebit,2>& plan) {
 for(const auto& p:plan) { auto* item=client.GetInv().GetItem(p.slot);if(item!=p.item || !item || item->GetSerialNumber()!=p.serial || item->GetCharges()!=p.charges || !item->GetItem() || item->GetItem()->ID!=p.id)return false; }
 return true;
}
void ReloadRequired(Client& client) {
 client.RejectInventorySessionAdmission();
 client.Kick("Campsite inventory outcome requires authoritative reload");
}
}
bool HasFellowshipCampsiteMaterials(Client& client,const Camps::Definition& definition) {
 std::array<MaterialDebit,2> plan{};return Stage(client,definition,plan);
}
Fellowship::Result<Fellowship::Snapshot> CreatePaidFellowshipCampsite(Client& client,const Camps::Definition& definition,const Fellowship::Campfire& fire,uint64_t now,uint64_t expected_revision,uint32_t expected_fellowship_id) {
 using Error=Fellowship::Error;
 if(definition.id!=fire.type || !expected_revision || !expected_fellowship_id)return {{},Error::InvalidArgument};
 std::array<MaterialDebit,2> plan{};
 if(!Stage(client,definition,plan))return {{},Error::InvalidArgument};
 const auto ticket=client.InventorySessionReleaseTicket();
 const auto lease=ticket?ticket->SnapshotFor(client):std::nullopt;
 if(!lease || !ticket->HeldFor(client) || !client.HasInventorySessionReservation())return {{},Error::InvalidArgument};
 const auto* cfg=EQEmuConfig::get();if(!cfg)return {{},Error::DatabaseFailure};
 Database exclusive;
 if(!exclusive.Connect(cfg->DatabaseHost,cfg->DatabaseUsername,cfg->DatabasePassword,cfg->DatabaseDB,cfg->DatabasePort,"fellowship-campsite-materials"))return {{},Error::DatabaseFailure};
 Fellowship::Service service(exclusive);
 auto result=service.SetCampfire(client.CharacterID(),fire,now,[&](Database& db) {
  if(!ticket->HeldFor(client) || client.InventorySessionReleaseTicket()!=ticket || !Frame(client,plan))return false;
  const auto current=ticket->SnapshotFor(client);
  if(!current || current->epoch!=lease->epoch || current->token!=lease->token)return false;
  std::array<Housing::LiveItems::InventoryRow,2> rows;
  for(size_t i=0;i<plan.size();++i) {
   auto selected=db.QueryDatabase("SELECT "+std::string(Housing::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+std::to_string(client.CharacterID())+" AND slot_id="+std::to_string(plan[i].slot),false);
   if(!selected.Success() || selected.RowCount()!=1)return false;
   rows[i]=Housing::InventoryRowDB::Decode(selected.begin());
   if(!Housing::InventoryProjection::Matches(rows[i],*plan[i].item))return false;
  }
  return Camps::ConsumeMaterialsUnderTransaction(db,definition.id,*lease,rows);
 },expected_revision,expected_fellowship_id);
 if(result.error==Error::CommitUncertain) { ReloadRequired(client);return result; }
 if(!result)return result;
 // No packets, quests or yielding occurred before COMMIT. Apply the already
 // durable decrements without issuing another database inventory write.
 if(!ticket->HeldFor(client) || !Frame(client,plan)) { ReloadRequired(client);return {{},Error::CommitUncertain}; }
 for(const auto& p:plan)client.DeleteItemInInventory(p.slot,1,true,false);
 return result;
}
