#pragma once
#include <cstdint>
#include <memory>
class Client;
namespace EQEmu::SharedCore::InventorySessions {class ZoneReservation;}
namespace EQ { class ItemInstance; }
namespace FireRunActionSource {class NativeRunActionCustody;class OriginalDoorUse;class PreparedRunDoorPlan;enum class DoorActionResult:uint8_t;}
namespace FireRunActionSource {class NativeRunTerminalCustody;class OriginalMoldTrade;class PreparedTerminalChestPlan;enum class OriginalTerminalReturnResult;}
namespace FirePersonalOccurrenceSource {
struct Binding;
// Client-owned, feature-private native input state. No packet/Lua constructor.
class NativeItemCustody final {
 struct PacketFence {uint64_t sequence=0;bool returned=false;};
public:
 class ClickTicket final {
  friend class NativeItemCustody;
  std::shared_ptr<PacketFence> fence_;
 public:
  ClickTicket()=default;
  ~ClickTicket(){if(fence_)fence_->returned=true;}
  ClickTicket(const ClickTicket&)=delete;
  ClickTicket& operator=(const ClickTicket&)=delete;
 };
public:
 class FinishTicket final {
  friend class NativeItemCustody;
  std::shared_ptr<PacketFence> packet_;
  std::shared_ptr<EQEmu::SharedCore::InventorySessions::ZoneReservation> session_;
  const Client* original_=nullptr;
  uint32_t character_=0;
 public:
  FinishTicket()=default;
  FinishTicket(const FinishTicket&)=delete;
  FinishTicket& operator=(const FinishTicket&)=delete;
  bool Selected()const noexcept{return bool(packet_);}
 };
 // The actual ordinary native charge call enters this stack scope. Empty
 // tickets are ordinary calls. Only FinishEntry may fill a selected ticket.
 class DeleteScope final {
  friend class NativeItemCustody;
  const FinishTicket& ticket_;
  DeleteScope* previous_;
  bool entered_=false;
  static thread_local DeleteScope* current_;
 public:
  explicit DeleteScope(const FinishTicket&) noexcept;
  ~DeleteScope();
  DeleteScope(const DeleteScope&)=delete;
  DeleteScope& operator=(const DeleteScope&)=delete;
 };
private:
 friend class FireRunActionSource::NativeRunActionCustody;
 friend class FireRunActionSource::NativeRunTerminalCustody;
 static FireRunActionSource::OriginalTerminalReturnResult MoldReturnSelected(Client&,
  std::unique_ptr<FireRunActionSource::OriginalMoldTrade>,std::unique_ptr<FireRunActionSource::PreparedTerminalChestPlan>);
 static FireRunActionSource::DoorActionResult DoorSelected(Client&,
  std::unique_ptr<FireRunActionSource::OriginalDoorUse>,std::unique_ptr<FireRunActionSource::PreparedRunDoorPlan>);
 static bool PublishSelected(Client&,bool client_update);
 struct State;
 std::unique_ptr<State> state_;
 NativeItemCustody();
 static NativeItemCustody* Owned(Client&);
 bool Capture(Client&,int16_t,const EQ::ItemInstance*,uint32_t,bool);
 bool Revalidate(Client&)const;
public:
 ~NativeItemCustody();
 NativeItemCustody(const NativeItemCustody&)=delete;
 NativeItemCustody& operator=(const NativeItemCustody&)=delete;
 static void Accepted(Client&,const Binding&);
 static bool ReloadAccepted(Client&);
 // Return true for ordinary items or a qualified selected input. False means
 // reject before any spend; never fall through to the old Fire cast/debit.
 static bool Click(Client&,int16_t,const EQ::ItemInstance*,uint32_t,bool,ClickTicket&);
 static bool Launch(Client&,int16_t,int32_t,uint32_t,const ClickTicket&);
 static bool Start(Client&,int16_t,int32_t,uint32_t);
 static bool FinishEntry(Client&,int16_t,int32_t,int16_t,FinishTicket&);
 static bool Finished(FinishTicket&,bool);
 static bool StillLive(const FinishTicket&) noexcept;
 static bool DeleteSelected(Client&,int16_t,int16_t,bool,bool);
 static void Cancel(Client&);
};
}
