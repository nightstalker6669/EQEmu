#pragma once
// Fire's selected native roots only. Client-owned intrusive membership; no
// generic transaction/session framework, timers, timeout or ownership takeover.
#include <cstdint>
#include <vector>
class SharedDatabase;
namespace EQ {class InventoryProfile;class ItemInstance;}
namespace FirePersonalOccurrenceSource {
class NativeItemCustody;
class NativeInventoryFence final {
 friend class NativeItemCustody;
 NativeInventoryFence()=default;
 const EQ::InventoryProfile* inventory_=nullptr;
 uint32_t character_=0;
 std::vector<int16_t> roots_;
 std::vector<const EQ::ItemInstance*> instances_;
 bool cursor_=false,attached_=false,launched_=false;
 const bool* native_packet_returned_=nullptr;
 bool Active()const noexcept {
  return attached_&&!(native_packet_returned_&&*native_packet_returned_&&!launched_);
 }
 const SharedDatabase* writer_=nullptr;
 NativeInventoryFence* next_=nullptr;
 static thread_local NativeInventoryFence* first_;
 void Attach() noexcept;
 void Detach() noexcept;
public:
 ~NativeInventoryFence(){Detach();}
 NativeInventoryFence(const NativeInventoryFence&)=delete;
 NativeInventoryFence& operator=(const NativeInventoryFence&)=delete;
 static bool ItemAllowed(const EQ::ItemInstance*) noexcept;
 static bool SlotAllowed(const EQ::InventoryProfile*,int16_t) noexcept;
 static bool ProfileAllowed(const EQ::InventoryProfile*) noexcept;
 static bool SaveAllowed(const SharedDatabase*,uint32_t,int16_t,bool whole_cursor=false) noexcept;
};
}
