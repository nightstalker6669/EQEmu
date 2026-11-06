// Included once by shared_task_manager.cpp. Actual native state, not a shadow
// task implementation, supplies the snapshot committed by the inventory owner.
#include "fungal_item_world_source.h"
namespace FungalItemWorldSource {
thread_local PacketScope* PacketScope::current_=nullptr;
thread_local NativeStage* NativeStage::current_=nullptr;
PacketScope::PacketScope(const ServerPacket& packet):previous_(current_) {
 current_=this;
 if(packet.opcode!=ServerOP_SharedTaskUpdate||packet.size<sizeof(ServerSharedTaskActivityUpdate_Struct))return;
 const auto* p=reinterpret_cast<const ServerSharedTaskActivityUpdate_Struct*>(packet.pBuffer);
 selected_=FungalItemWire::Correlated(p->task_id,p->activity_id);if(!selected_)return;
 character_=p->source_character_id;activity_=p->activity_id;count_=p->done_count;
 const bool pure=FungalItemWire::PureHail(p->task_id,p->activity_id);
 valid_=(pure?FungalItemWire::ReadHeader(packet.pBuffer,packet.size,sizeof(*p),original_):
  FungalItemWire::Read(packet.pBuffer,packet.size,sizeof(*p),original_))&&
  (pure?!original_.outcome:bool(original_.outcome))&&
  original_.recipient==character_&&count_==(activity_==5?2u:1u)&&!p->ignore_quest_update;
}
PacketScope::~PacketScope(){current_=previous_;}
NativeStage::NativeStage(Database& db,SharedTask* actual,uint32_t character,uint32_t task,
 uint32_t activity,uint32_t count):previous_(current_) {
 current_=this;selected_=FungalItemWire::Correlated(task,activity);if(!selected_)return;
 const auto* packet=PacketScope::current_;
 if(!packet||!packet->selected_||!packet->valid_||packet->character_!=character||
  packet->activity_!=activity||packet->count_!=count||!actual||actual->GetTaskData().id!=920143||
  uint64_t(actual->GetDbSharedTask().id)!=packet->original_.run)return;
 bool active=false;bool terminal_hidden=false;
 for(const auto& a:actual->m_shared_task_activity_state) {
  if(a.activity_id==activity)active=a.done_count==0&&a.max_done_count==count&&a.activity_state==ActivityState::ActivityActive;
  if(a.activity_id==11)terminal_hidden=a.done_count==0;
 }
 if(!active||!terminal_hidden)return;
 try {
  original_=std::unique_ptr<OriginalWorldUpdate>(new OriginalWorldUpdate);
  original_->original_=packet->original_;original_->activity_=activity;original_->count_=count;
  original_->actual_=actual;
  // Core locks persisted run/member/stages + exact committed finite outcome
  // BEFORE the original World mutates its own native activity vector.
  transaction_=FungalInventory::WorldCustody::Prepare(db,*original_);
 }catch(...){transaction_.reset();original_.reset();}
}
NativeStage::~NativeStage(){transaction_.reset();current_=previous_;}
bool NativeStage::SelectedSave(uint64_t run) {
 return current_&&current_->selected_&&current_->original_&&
  current_->original_->original_.run==run;
}
bool NativeStage::CommitActualSave(Database& db,uint64_t run,
 const std::vector<SharedTaskActivityStateRepository::SharedTaskActivityState>& rows) {
 auto* f=current_;
 if(!SelectedSave(run)||!f->transaction_||f->ack_)return false;
 try {
  auto ack=FungalInventory::WorldCustody::CommitActualSave(db,*f->transaction_,*f->original_,rows);
  const auto& o=f->original_->original_;
  if(!ack||ack->original_.run!=o.run||ack->original_.outcome!=o.outcome||
   ack->original_.accepted!=o.accepted||ack->original_.recipient!=o.recipient||
   ack->activity_!=f->original_->activity_||ack->count_!=f->original_->count_)return false;
  f->ack_=std::move(ack);return true;
 }catch(...){return false;}
}
bool NativeStage::StampActualAck(uint64_t run,uint32_t activity,FungalItemWire::Trailer& out) {
 const auto* f=current_;
 if(!f||!f->ack_||f->ack_->original_.run!=run||f->ack_->activity_!=activity)return false;
 out=f->ack_->original_;return true;
}
} // namespace FungalItemWorldSource
