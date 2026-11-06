#include "fungal_unknown_inventory.h"
#include "fire_native_inventory_fence.h"
#include "data_verification.h"
#include "inventory_profile.h"
#include <algorithm>
namespace FirePersonalOccurrenceSource {
thread_local NativeInventoryFence* NativeInventoryFence::first_=nullptr;
namespace {
int16_t Root(int16_t slot) noexcept {
 const auto parent=EQ::InventoryProfile::CalcSlotId(slot);
 return parent==INVALID_INDEX?slot:parent;
}
bool Cursor(int16_t slot) noexcept {
 return slot==EQ::invslot::slotCursor ||
  EQ::ValueWithin(slot,EQ::invbag::CURSOR_BAG_BEGIN,EQ::invbag::CURSOR_BAG_END);
}
}
void NativeInventoryFence::Attach() noexcept {
 if(attached_)return;
 next_=first_;first_=this;attached_=true;
}
void NativeInventoryFence::Detach() noexcept {
 if(!attached_)return;
 auto** link=&first_;
 while(*link&&*link!=this)link=&(*link)->next_;
 if(*link)*link=next_;
 attached_=false;next_=nullptr;writer_=nullptr;
}
bool NativeInventoryFence::ItemAllowed(const EQ::ItemInstance* item) noexcept {
 if(!FungalInventory::UnknownInventory::ItemAllowed(item))return false;
 for(auto* f=first_;f;f=f->next_)
  if(f->Active()&&std::find(f->instances_.begin(),f->instances_.end(),item)!=f->instances_.end())return false;
 return true;
}
bool NativeInventoryFence::SlotAllowed(const EQ::InventoryProfile* inv,int16_t slot) noexcept {
 if(!FungalInventory::UnknownInventory::ProfileAllowed(inv))return false;
 for(auto* f=first_;f;f=f->next_)if(f->Active()&&f->inventory_==inv) {
  if((f->cursor_&&Cursor(slot))||std::find(f->roots_.begin(),f->roots_.end(),Root(slot))!=f->roots_.end())return false;
 }
 return true;
}
bool NativeInventoryFence::ProfileAllowed(const EQ::InventoryProfile* inv) noexcept {
 if(!FungalInventory::UnknownInventory::ProfileAllowed(inv))return false;
 for(auto* f=first_;f;f=f->next_)if(f->Active()&&f->inventory_==inv)return false;
 return true;
}
bool NativeInventoryFence::SaveAllowed(const SharedDatabase* db,uint32_t character,int16_t slot,bool whole_cursor) noexcept {
 if(!FungalInventory::UnknownInventory::SaveAllowed(character))return false;
 for(auto* f=first_;f;f=f->next_)if(f->Active()&&f->character_==character) {
  const bool selected=(f->cursor_&&(whole_cursor||Cursor(slot)))||
   std::find(f->roots_.begin(),f->roots_.end(),Root(slot))!=f->roots_.end();
  // This grant is set only around the original selected writer's synchronous
  // SaveInventory/SaveCursor calls, with no callbacks, sends or event pumping.
  if(selected&&f->writer_!=db)return false;
 }
 return true;
}
}
