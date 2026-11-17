#pragma once
#include "client.h"
#include "npc.h"
#include "task_client_state.h"

// Original nonspell quest-item click. The server already dispatches ITEM_CLICK
// before spell/effect handling for ordinary carried click slots. Do not invent
// an Akromid spell or alter item17345's authentic no-effect definition.
namespace SodFieldOfScaleCrystalSource {
class Frame final {
 friend class ::Client; // Only original Client opcode handlers can construct.
 Client* client_=nullptr;
 const EQ::ItemInstance* item_=nullptr;
 int32_t slot_=-1,serial_=0;
 uint32_t target_=0,instance_=0;
 uint64_t run_=0;
 bool valid_=false,used_=false;
 Frame* previous_=nullptr;
 inline static thread_local Frame* current_=nullptr;
 Frame(Client& c,int32_t slot,const EQ::ItemInstance* item,uint32_t target) noexcept
 :client_(&c),item_(item),slot_(slot),serial_(item?item->GetSerialNumber():0),
  target_(target),instance_(c.GetInstanceID()),run_(static_cast<uint64_t>(c.GetSharedTaskId())),previous_(current_) {
  current_=this;
  auto* task=c.GetTaskState();
  valid_=item && item->GetID()==17345 && serial_>0 && slot>=0 &&
   c.GetInv().GetItem(slot)==item && target && target<=0xffffu && instance_ && c.GetZoneID()==453 &&
   c.GetSharedTaskId()>0 && task && task->GetActiveSharedTask().task_id==920213 &&
   c.IsTaskActivityActive(920213,1);
 }
public:
 ~Frame() noexcept {current_=previous_;}
 Frame(const Frame&)=delete;
 Frame& operator=(const Frame&)=delete;
 // The Lua provider returns this target only during the same ORIGINAL click.
 // SceneState additionally validates actual native occurrence/DZ membership.
 static uint32_t ClaimTarget(Client& c) noexcept {
  auto* f=current_;
  if(!f || !f->valid_ || f->used_ || f->client_!=&c ||
   c.GetInstanceID()!=f->instance_ || c.GetZoneID()!=453 || c.GetSharedTaskId()<=0 ||
   static_cast<uint64_t>(c.GetSharedTaskId())!=f->run_ ||
   c.GetInv().GetItem(f->slot_)!=f->item_ || f->item_->GetSerialNumber()!=f->serial_ ||
   f->item_->GetID()!=17345 || !c.IsTaskActivityActive(920213,1))return 0;
  f->used_=true;return f->target_;
 }
};
}
