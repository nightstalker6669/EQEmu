namespace KithicorPoisonTrade {class Writer;}
namespace RechargeRecipe8 {class Custody;}
namespace TrophyCertificate55 {class Custody;}
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace EQ {class InventoryProfile;class ItemInstance;}
namespace FungalInventory {
class NativeCustody;
// Existing finite native writers may attach this shared admission hold. It is activated
// on unknown strict commit OR a known original shared-delivery continuation,
// with all allocations made before persistence. Only the actual writer may
// release a continuation after verified physical settlement; unknown outcomes
// remain fenced through actual complete-delete recovery.
// No new authority, task grant API, whitelist or Fire action-table reuse.
class UnknownInventory final {
 friend class NativeCustody;
 friend class KithicorPoisonTrade::Writer;
 friend class TrophyCertificate55::Custody;
 friend class RechargeRecipe8::Custody;
 UnknownInventory()=default;
 uint32_t character_=0;
 const EQ::InventoryProfile* inventory_=nullptr;
 std::vector<const EQ::ItemInstance*> items_;
 UnknownInventory* next_=nullptr;
 inline static thread_local UnknownInventory* first_=nullptr;
 inline static thread_local const UnknownInventory* writer_grant_=nullptr;
 bool continuation_=false;
 // Only the SAME actual writer may resume a known original continuation. No
 // Lua/event pumping or native semantic callbacks may run while this is set.
 struct WriterGrant {
  const UnknownInventory* old_;
  explicit WriterGrant(const UnknownInventory* h):old_(writer_grant_){writer_grant_=h;}
  ~WriterGrant(){writer_grant_=old_;}
 };
 void ReleaseKnownContinuation()noexcept {
  if(!continuation_)return;
  auto** p=&first_;while(*p){if(*p==this){*p=next_;delete this;return;}p=&(*p)->next_;}
 };

 void Attach()noexcept{next_=first_;first_=this;}
public:
 ~UnknownInventory()=default;
 UnknownInventory(const UnknownInventory&)=delete;
 UnknownInventory& operator=(const UnknownInventory&)=delete;
 static bool ItemAllowed(const EQ::ItemInstance* item)noexcept{for(auto* p=first_;p;p=p->next_)if(p!=writer_grant_&&std::find(p->items_.begin(),p->items_.end(),item)!=p->items_.end())return false;return true;}
 static bool ProfileAllowed(const EQ::InventoryProfile* inv)noexcept{for(auto* p=first_;p;p=p->next_)if(p!=writer_grant_&&p->inventory_==inv)return false;return true;}
 static bool SaveAllowed(uint32_t character)noexcept{for(auto* p=first_;p;p=p->next_)if(p!=writer_grant_&&p->character_==character)return false;return true;}
 // Actual InventoryProfile destructor only: allow its owned trees to be freed,
 // while character-wide stale saves remain blocked through full Client delete.
 static void Destructing(const EQ::InventoryProfile* inv)noexcept{for(auto* p=first_;p;p=p->next_)if(p->inventory_==inv){p->inventory_=nullptr;p->items_.clear();}}
 // Existing ZoneReservation complete-delete boundary only. Never on a timer,
 // disconnect callback, frame cancellation, or an unknown save result.
 static void AfterCompleteDelete(uint32_t character)noexcept{auto** p=&first_;while(*p){if((*p)->character_==character){auto* old=*p;*p=old->next_;delete old;}else p=&(*p)->next_;}}
};
}
