#pragma once
#include "../common/housing/live_item_service.h"
#include "../common/housing/region_wire.h"
#include <memory>
#include "inventory_session_owner.h"
class Client; class Database;
namespace HousingRecipe301 {
inline bool Selected(uint32_t id) { return id>=3102767 && id<=3102779; }
inline uint32_t Output(uint32_t id) { switch(id) {
 case 3102767:return 61642;
 case 3102768:return 61659;
 case 3102769:return 61669;
 case 3102770:return 61674;
 case 3102771:return 61703;
 case 3102772:return 61713;
 case 3102773:return 61724;
 case 3102774:return 61738;
 case 3102775:return 61757;
 case 3102776:return 61765;
 case 3102777:return 61769;
 case 3102778:return 61792;
 case 3102779:return 61809;
 default:return 0; } }
struct Binding {
 Housing::LiveItems::Stored stored;
 std::shared_ptr<EQEmu::SharedCore::InventorySessions::ZoneReservation> owner;
 uint32_t actor=0,account=0,native_plot=0,zone=0,instance=0,version=0;
 uint64_t catalog_revision=0,acl_revision=0;
 std::string guid;
 std::vector<Housing::RegionWire::Region> regions;
};
// Dedicated READ/row-lock connection, never the native inventory writer's connection.
// Locks are held through ingredient debit and released before ordinary output/task hooks.
class Use {
public:
 static std::unique_ptr<Use> Acquire(Client&,const Binding&) noexcept;
 ~Use() noexcept;
 bool Held() noexcept;
 bool AdmitDebit() noexcept; // once, immediately before the first native debit
private:
 Use()=default;
 std::unique_ptr<Database> db_;
 uint64_t connection_=0;
 bool debit_admitted_=false;
};
}
