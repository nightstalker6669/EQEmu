// Native selected writer and custody source proposal. Not installed/built or
// activation-ready; initial admission and serving activation remain unselected.
#include "client.h"
#include "task_client_state.h"
#include "zonedb.h"
#include "zone.h"
#include "fire_personal_item_use.h"
#include "entity.h"
#include "fire_native_item_custody.h"
#include "fire_original_item_snapshot.h"
#include "fire_native_published_use.h"
#include "fire_shared_transform_published.h"
#include "fire_prepared_construct_plan.h"
#include "inventory_session_owner.h"
#include "../common/fire_native_inventory_fence.h"
#include "../common/housing/inventory_row_db.h"
#include "../common/housing/inventory_projection.h"
#include "fire_run_action_custody.h"
#include "fire_run_action_scene.h"
#include "fire_run_item_origin.h"
#include "fire_material_loot_source.h"
#include "fire_run_terminal_custody.h"
#include "fire_run_terminal_scene.h"
#include <utility>
#include "../common/util/uuid.h"
#include "../common/dynamic_zone_base.h"
#include <charconv>
#include <cstring>
#include <list>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <vector>
extern Zone* zone;
extern ZoneDatabase database;
#include "fire_run_action_custody_impl.inc"
namespace FireRunActionSource { namespace {
void RequireActualGuardianLoot(Database&,const RunLootFacts&);
}}
#include "fire_run_item_origin_impl.inc"
#include "fire_run_terminal_custody_impl.inc"
namespace FirePersonalOccurrenceSource {
PublishedNativeUse::PublishedNativeUse(const Client& owner,const OriginalItemUse& original,
 uint64_t sequence,uint32_t output_item,int32_t output_serial,int16_t output_slot,
 int16_t output_charges,std::unique_ptr<PreparedConstructPlan> prepared_plan) noexcept
 :owner_(&owner),prepared_plan_(std::move(prepared_plan)),
  view_{original,sequence,output_item,output_serial,output_slot,output_charges,prepared_plan_.get()} {}
PublishedNativeUse::PublishedNativeUse(PublishedNativeUse&& other) noexcept
 :owner_(other.owner_),prepared_plan_(std::move(other.prepared_plan_)),
  view_(other.view_),taken_(other.taken_) {
 view_.prepared_construct_plan=prepared_plan_.get();
 other.owner_=nullptr;other.taken_=true;other.view_.prepared_construct_plan=nullptr;
}
PublishedNativeUse::~PublishedNativeUse()=default;

namespace {
int RuleIndex(uint32_t task) {
 switch(task){case 920167:return 0;case 920168:return 1;case 920169:return 2;default:return -1;}
}
bool OriginalClickItem(uint32_t item) {
 switch(item){case 50745:case 50746:case 50747:case 50749:case 50750:case 50752:
  case 47760:case 47752:case 47756:return true;default:return false;}
}
int16_t RootSlot(int16_t slot) {
 if(EQ::ValueWithin(slot,EQ::invbag::GENERAL_BAGS_BEGIN,EQ::invbag::GENERAL_BAGS_END))
  return EQ::InventoryProfile::CalcSlotId(slot);
 return slot;
}
bool CarriedGeneral(const EQ::InventoryProfile& inv,int16_t slot) {
 const auto root=RootSlot(slot);
 return EQ::ValueWithin(root,EQ::invslot::GENERAL_BEGIN,EQ::invslot::GENERAL_END)&&
  inv.GetLookup()&&((uint64_t(1)<<root)&inv.GetLookup()->PossessionsBitmask);
}
}
struct NativeItemCustody::State {
 enum class Phase { Captured,LaunchReady,Casting,Finishing,SpellSucceeded,DoorReady,Rejected,Unknown,Committed };
 struct Take {int16_t slot;int16_t quantity;const EQ::ItemInstance* original;uint32_t item_id;};
 struct Click {
  Click(std::optional<Context> value,std::shared_ptr<PacketFence> p,uint64_t seq,uint32_t who,uint32_t to,uint32_t z,uint32_t instance_id,int16_t where)
   :context(std::move(value)),packet(std::move(p)),sequence(seq),caster(who),target(to),zone(z),instance(instance_id),slot(where){}
  std::optional<Context> context;
  std::optional<FireSharedTransformSource::OriginalSharedTransformUse> shared_use;
  std::unique_ptr<FireRunActionSource::OriginalDoorUse> door_use;
  std::unique_ptr<FireRunActionSource::OriginalHelperUse> helper_use;
  std::unique_ptr<FireRunActionSource::PreparedRunDoorPlan> door_plan;
  std::unique_ptr<FireRunActionSource::PreparedRunHelperPlan> helper_plan;
  std::unique_ptr<FireRunActionSource::RunLatchSnapshot> latch;
  std::unique_ptr<FireRunActionSource::PreparedRunActionOrigins> action_origins;
  std::unique_ptr<FireRunActionSource::PreparedRunTransformOrigins> transform_origins;
  bool inventory_mutates=true;
  int32_t native_click_spell=7651;
  std::shared_ptr<PacketFence> packet;
  uint64_t sequence;
  uint32_t caster,target,zone,instance;
  int16_t slot;
  int16_t debit_slot=-1;
  Phase phase=Phase::Captured;
  std::unique_ptr<OriginalItemSnapshot> clicked;
  std::map<int16_t,std::unique_ptr<OriginalItemSnapshot>> roots;
  std::vector<std::unique_ptr<OriginalItemSnapshot>> cursor;
  std::vector<Take> b;
  std::unique_ptr<EQ::ItemInstance> output;
  std::array<unsigned char,sizeof(EQ::ItemData)> output_definition{};
  int16_t output_slot=-1;
  bool cursor_participates=false;
  std::optional<OriginalItemUse> original_use;
  std::shared_ptr<Housing::InventorySessions::ZoneReservation> session;
  std::optional<Housing::InventorySessions::Lease> lease;
  NativeInventoryFence fence;
  std::unique_ptr<EQ::InventoryProfile> staged;
  std::vector<uint8_t> save_results;
  std::vector<std::unique_ptr<EQApplicationPacket>> packets;
  bool persistence_entered=false,commit_attempted=false;

 };
 enum class MoldPhase {Captured,Committed,Unknown,Rejected};
 struct MoldReturn {
  MoldPhase phase=MoldPhase::Captured;
  std::unique_ptr<FireRunActionSource::PublishedTerminalReturnReceipt::State> receipt;
  std::unique_ptr<OriginalItemSnapshot> mold;
  EQ::ItemInstance* original=nullptr;
  std::unique_ptr<EQ::ItemInstance> spent;
  std::string token;
  std::shared_ptr<Housing::InventorySessions::ZoneReservation> session;
  std::optional<Housing::InventorySessions::Lease> lease;
  NativeInventoryFence fence;
  bool persistence_entered=false,commit_attempted=false;
 };
 std::unique_ptr<MoldReturn> mold_return;
 std::array<std::optional<Binding>,3> accepted;
 uint64_t sequence=0;
 std::unique_ptr<Click> click;
};
NativeItemCustody::NativeItemCustody():state_(std::make_unique<State>()){}
NativeItemCustody::~NativeItemCustody()=default;
NativeItemCustody* NativeItemCustody::Owned(Client& c){return c.m_fire_item_custody.get();}
void NativeItemCustody::Accepted(Client& c,const Binding& b) {
 const auto n=RuleIndex(b.task_id);if(n<0||b.character_id!=c.CharacterID()||!b.occurrence_id)return;
 // Caller supplies ONLY the actual successful native BindAccepted receipt.
 // No Reserve/Ensure/history reconstruction is performed here.
 if(!c.m_fire_item_custody)c.m_fire_item_custody.reset(new NativeItemCustody);
 auto& owner=*c.m_fire_item_custody;
 if(owner.state_->click&&owner.state_->click->phase==State::Phase::Unknown)return;
 owner.state_->accepted[n]=b;
}
bool NativeItemCustody::ReloadAccepted(Client& c) {
 if(!c.CharacterID()||!c.task_state||!NativeInventoryFence::ProfileAllowed(&c.m_inv))return false;
 if(!c.m_fire_item_custody)c.m_fire_item_custody.reset(new NativeItemCustody);
 auto& owner=*c.m_fire_item_custody;
 // Reload acceptance/stage context only. No native use/tree/output/death proof,
 // cast ticket, Reserve/Ensure, grant, task save or fresh acceptance is revived.
 if(owner.state_->click) {
  if(owner.state_->click->fence.Active())return false;
  owner.state_->click.reset();
 }
 owner.state_->accepted={};
 std::array<std::optional<Binding>,3> restored;
 try {
  for(int slot=0;slot<MAXACTIVEQUESTS;++slot) {
   const auto* current=c.task_state->GetClientTaskInfo(TaskType::Quest,slot);
   if(!current||current->task_id<=0||current->slot!=slot||current->accepted_time<=0||current->was_rewarded)continue;
   const auto index=RuleIndex(uint32_t(current->task_id));if(index<0)continue;
   if(restored[index])return false;
   auto rows=database.QueryDatabase(fmt::format(
    "SELECT r.occurrence_id FROM character_task_reward_instances r "
    "JOIN character_tasks ct ON ct.charid=r.character_id AND ct.taskid=r.task_id "
    "AND ct.acceptedtime=r.accepted_time WHERE r.character_id={} AND r.task_id={} "
    "AND r.accepted_time={} AND ct.slot={} AND ct.type=2 AND ct.was_rewarded=0",
    c.CharacterID(),current->task_id,current->accepted_time,slot),false);
   if(!rows.Success()||rows.RowCount()>1)return false;
   if(!rows.RowCount())continue; // legacy/unbound rows never get a new occurrence
   auto row=rows.begin();if(!row[0])return false;
   const std::string value=row[0];size_t end=0;
   if(value.empty()||value.find_first_not_of("0123456789")!=std::string::npos)return false;
   const auto occurrence=std::stoull(value,&end);if(!occurrence||end!=value.size())return false;
   Binding binding{c.CharacterID(),uint32_t(current->task_id),slot,current->accepted_time,occurrence};
   if(!MatchesLocal(binding,c.CharacterID(),TaskType::Quest,*current))return false;
   restored[index]=binding;
  }
 }catch(...){return false;}
 owner.state_->accepted=restored;
 return true;
}
bool NativeItemCustody::Capture(Client& c,int16_t slot,const EQ::ItemInstance* actual,
 uint32_t target,bool supported) {
 if(!actual||!actual->GetItem()||!OriginalClickItem(actual->GetID())||!supported||
  (!CarriedGeneral(c.m_inv,slot)&&slot!=EQ::invslot::slotCursor)||
  !zone||!c.task_state||!c.GetID()||state_->sequence==std::numeric_limits<uint64_t>::max())return false;
 if(state_->click&&(state_->click->phase==State::Phase::Captured||
  state_->click->phase==State::Phase::LaunchReady)&&state_->click->packet&&
  state_->click->packet->returned)state_->click->phase=State::Phase::Rejected;
 if(state_->click&&state_->click->phase!=State::Phase::Rejected&&
  state_->click->phase!=State::Phase::Committed)return false;
 const auto shared_rule=FireSharedTransformSource::OriginalRule(actual->GetID());
 const bool is_helper=actual->GetID()==47756;
 std::optional<Context> context;
 if(!shared_rule&&!is_helper)for(const auto& bound:state_->accepted) {
  if(!bound)continue;
  const auto rule=OriginalRule(bound->task_id);if(!rule)continue;
  const bool combine=actual->GetID()==rule->ingredient_a;
  if(!combine&&actual->GetID()!=rule->inactive_item)continue;
  if(bound->slot<0||bound->slot>=MAXACTIVEQUESTS)continue;
  const auto* current=c.task_state->GetClientTaskInfo(TaskType::Quest,bound->slot);
  if(!current||current->activity[0].activity_state!=ActivityCompleted||
   current->activity[1].activity_state!=ActivityCompleted)continue;
  context=ForConsumer(database,*bound,c.CharacterID(),TaskType::Quest,*current,
   combine?Consumer::CombineIngredients:Consumer::ActivateConstruct);
  if(context)break;
 }
 if(!context&&!shared_rule&&!is_helper)return false;
 std::unique_ptr<FireRunActionSource::OriginalHelperUse> helper_use;
 std::optional<OriginalItemUse> use;
 std::optional<FireSharedTransformSource::OriginalSharedTransformUse> shared_use;
 if(context)use=CaptureOriginalItemUse(*context,*actual,slot,supported,c.GetID(),target,
  zone->GetZoneID(),zone->GetInstanceID());
 else if(is_helper)helper_use=FireRunActionSource::NativeRunHelperSource::CaptureActual(c,*actual,slot,supported,target);
 else shared_use=FireSharedTransformSource::NativeSharedTransform::CaptureActual(c,*actual,slot,supported,target);
 if((!use&&!shared_use&&!helper_use)||actual->GetCharges()!=1||actual->IsStackable()||!actual->IsExpendable())return false;
 auto click=std::make_unique<State::Click>(context,std::make_shared<PacketFence>(),++state_->sequence,
  c.GetID(),target,zone->GetZoneID(),zone->GetInstanceID(),slot);
 click->packet->sequence=click->sequence;
 click->original_use=use;click->shared_use=shared_use;click->helper_use=std::move(helper_use);
 if(click->helper_use) {
  click->latch=FireRunActionSource::NativeRunActionCustody::PreviewActual(*click->helper_use);
  click->action_origins=FireRunActionSource::NativeRunItemOrigins::PrepareActionActual(c,nullptr,click->helper_use.get());
  if(!click->latch||!click->action_origins)return false;
 }
 click->native_click_spell=context?7651:actual->GetItem()->Click.Effect;
 if(click->native_click_spell<=0)return false;
 click->session=c.InventorySessionReleaseTicket();
 click->lease=click->session?click->session->SnapshotFor(c):std::nullopt;
 if(!click->lease||!c.HasInventorySessionReservation())return false;

 click->clicked=OriginalItemSnapshot::Capture(slot,actual);if(!click->clicked)return false;
 const auto capture_root=[&](int16_t selected) {
  const auto root=RootSlot(selected);
  if(root==EQ::invslot::slotCursor){click->cursor_participates=true;return true;}
  if(click->roots.count(root))return true;
  auto snapshot=OriginalItemSnapshot::Capture(root,c.m_inv.GetItem(root));
  if(!snapshot)return false;
  click->roots.emplace(root,std::move(snapshot));return true;
 };
 if(!capture_root(slot))return false;
 if(context&&context->consumer==Consumer::CombineIngredients) {
  int need=10;
  for(const auto candidate:c.GetInventorySlots()) {
   if(need==0)break;
   if(candidate==slot||!CarriedGeneral(c.m_inv,candidate))continue;
   const auto* item=c.m_inv.GetItem(candidate);
   if(!item||item->GetID()!=context->rule.ingredient_b||!item->IsStackable()||
    item->GetCharges()<=0)continue;
   const auto quantity=static_cast<int16_t>(std::min<int>(need,item->GetCharges()));
   if(!capture_root(candidate))return false;
   click->b.push_back({candidate,quantity,item,context->rule.ingredient_b});need-=quantity;
  }
  if(need)return false;
 }
 if(shared_use) {
  const auto& rule=shared_use->shared.rule;
  if(rule.input_count!=2&&rule.input_count!=5)return false;
  // All remaining physical inputs come from this SAME holder's carried trees.
  // Each distinct input ID consumes one actual instance/unit, never a group copy.
  bool clicked_once=false;
  for(size_t i=0;i<rule.input_count;++i) {
   const auto input=rule.inputs[i];if(!input)return false;
   for(size_t j=0;j<i;++j)if(rule.inputs[j]==input)return false;
   if(input==actual->GetID()){if(clicked_once)return false;clicked_once=true;continue;}
   bool found=false;
   for(const auto candidate:c.GetInventorySlots()) {
    if(candidate==slot||!CarriedGeneral(c.m_inv,candidate))continue;
    const auto* item=c.m_inv.GetItem(candidate);
    if(!item||item->GetID()!=input||(item->IsStackable()&&item->GetCharges()<1))continue;
    if(!capture_root(candidate))return false;
    click->b.push_back({candidate,1,item,input});found=true;break;
   }
   if(!found)return false;
  }
  if(!clicked_once)return false;
 }
 if(shared_use||(context&&context->consumer==Consumer::CombineIngredients)) {
  const auto output=shared_use?shared_use->shared.rule.output_item:context->rule.inactive_item;
  const auto* definition=database.GetItem(output);
  if(!definition||c.CheckLoreConflict(definition))return false;
  if(shared_use&&(definition->Stackable ||
   (output==47762?(definition->MaxCharges!=0||definition->Click.Effect!=-1):
    (output!=47756||definition->MaxCharges!=1||definition->Click.Effect!=7651||definition->Click.Type!=3))))return false;
  if(!shared_use&&(definition->Stackable||definition->MaxCharges<=0||
   definition->Click.Effect!=7651||definition->Click.Type!=3))return false;
  // ONE actual output, canonical native charges are separate from physical yield.
  const int16_t canonical_charges=definition->Stackable?1:definition->MaxCharges;
  std::memcpy(click->output_definition.data(),definition,sizeof(EQ::ItemData));
  click->output.reset(database.CreateItem(definition,canonical_charges));
  if(!click->output||click->output->GetID()!=output||
   click->output->GetCharges()!=canonical_charges)return false;
  bool fits=CarriedGeneral(c.m_inv,slot);
  const auto root=RootSlot(slot);
  if(fits&&root!=slot) {
   const auto* parent=c.m_inv.GetItem(root);
   fits=parent&&parent->GetItem()&&
    EQ::InventoryProfile::CanItemFitInContainer(definition,parent->GetItem());
  }
  if(fits)click->output_slot=slot;
  else {click->output_slot=EQ::invslot::slotCursor;click->cursor_participates=true;}
 }
 if(shared_use) {
  std::vector<int16_t> inputs{slot};for(const auto& take:click->b)inputs.push_back(take.slot);
  click->transform_origins=FireRunActionSource::NativeRunItemOrigins::PrepareTransformActual(c,*shared_use,
   inputs,*click->output,click->sequence,click->output_slot);
  if(!click->transform_origins)return false;
 }
 if(click->cursor_participates) {
  // Capture the complete actual queue before callbacks. Capacity qualification
  // accounts for removing A only if A itself is the current cursor item.
  const auto old_count=c.m_inv.CursorSize();
  const auto add=click->output?1:0;
  const auto remove=slot==EQ::invslot::slotCursor?1:0;
  if(old_count+add-remove>EQ::invbag::CURSOR_BAG_COUNT)return false;
  for(auto it=c.m_inv.cursor_cbegin();it!=c.m_inv.cursor_cend();++it) {
   auto snapshot=OriginalItemSnapshot::Capture(EQ::invslot::slotCursor,*it);
   if(!snapshot)return false;
   click->cursor.push_back(std::move(snapshot));
  }
 }
 // Selected original trees are held through callbacks, cast, persistence and
 // publication. Never lock a whole zone or unrelated Client/root.
 click->fence.inventory_=&c.m_inv;click->fence.character_=c.CharacterID();
 click->fence.cursor_=click->cursor_participates;
 click->fence.native_packet_returned_=&click->packet->returned;
 const auto protect=[&](const auto& self,const EQ::ItemInstance* item,unsigned depth)->bool {
  // Existing native evolving/house identity writers are separate owners.
  // Unsupported selected tree shapes reject BEFORE spend, never shed identity.
  if(!item||item->IsEvolving()||item->GetEvolveUniqueID()||item->GetHousingHouseID()||item->GetHousingCrateID()||(depth&&item->IsClassBag()))return false;
  click->fence.instances_.push_back(item);
  for(const auto& child:*const_cast<EQ::ItemInstance*>(item)->GetContents())
   if(child.second&&!self(self,child.second,depth+1))return false;
  return true;
 };
 for(const auto& root:click->roots) {
  click->fence.roots_.push_back(root.first);
  if(!protect(protect,c.m_inv.GetItem(root.first),0))return false;
 }
 if(click->cursor_participates)for(auto it=c.m_inv.cursor_cbegin();it!=c.m_inv.cursor_cend();++it) {
  // Native SaveCursor shares its durable queue range with cursor bag children.
  // Non-container queue trees only; never assume unsupported queue capacity.
  if((*it)->IsClassBag()||!(*it)->GetContents()->empty()||!protect(protect,*it,0))return false;
 }
 state_->click=std::move(click);state_->click->fence.Attach();
 if(!Revalidate(c)){state_->click.reset();return false;}
 return true;
}
bool NativeItemCustody::Revalidate(Client& c)const {
 if(!state_->click||!zone||!c.task_state)return false;
 const auto& f=*state_->click;
 if(!f.session||c.InventorySessionReleaseTicket()!=f.session||!f.session->HeldFor(c))return false;
 if(unsigned(bool(f.context))+unsigned(bool(f.shared_use))+unsigned(bool(f.helper_use))+unsigned(bool(f.door_use))!=1)return false;
 const auto character=f.shared_use?f.shared_use->shared.character:f.helper_use?f.helper_use->Facts().mission.character:
  f.door_use?f.door_use->Facts().character:f.context->acceptance.character_id;
 const bool produces=f.shared_use||(f.context&&f.context->consumer==Consumer::CombineIngredients);
 if(produces) {
  const auto output=f.shared_use?f.shared_use->shared.rule.output_item:f.context->rule.inactive_item;
  const auto* definition=database.GetItem(output);
  if(!definition||std::memcmp(f.output_definition.data(),definition,sizeof(EQ::ItemData))||c.CheckLoreConflict(definition))return false;
 }
 if(c.GetID()!=f.caster||c.CharacterID()!=character||
  zone->GetZoneID()!=f.zone||zone->GetInstanceID()!=f.instance||
  (f.clicked&&!f.clicked->Matches(f.slot,c.m_inv.GetItem(f.slot))))return false;
 if(f.shared_use) {
  if(!FireSharedTransformSource::NativeSharedTransform::RevalidateActual(c,*f.shared_use))return false;
  if(!f.transform_origins||!FireRunActionSource::NativeRunItemOrigins::RevalidateTransformActual(c,*f.transform_origins))return false;
 }else if(f.helper_use) {
  if(!FireRunActionSource::NativeRunHelperSource::RevalidateActual(c,*f.helper_use)||!f.action_origins||
   !FireRunActionSource::NativeRunItemOrigins::RevalidateActionActual(c,*f.action_origins))return false;
 }else if(f.door_use) {
  if(!f.door_plan||!FireRunActionSource::NativeRunActionCustody::RevalidateDoorPlan(c,*f.door_plan)||
   (f.action_origins&&!FireRunActionSource::NativeRunItemOrigins::RevalidateActionActual(c,*f.action_origins)))return false;
 }else {
  const auto* current=c.task_state->GetClientTaskInfo(TaskType::Quest,f.context->acceptance.slot);
  if(!current||current->activity[0].activity_state!=ActivityCompleted||
   current->activity[1].activity_state!=ActivityCompleted)return false;
  if(!ForConsumer(database,f.context->acceptance,c.CharacterID(),TaskType::Quest,*current,f.context->consumer))return false;
 }
 for(const auto& entry:f.roots)
  if(!entry.second->Matches(entry.first,c.m_inv.GetItem(entry.first)))return false;
 if(f.cursor_participates) {
  if(c.m_inv.CursorSize()!=static_cast<int>(f.cursor.size()))return false;
  auto it=c.m_inv.cursor_cbegin();
  for(const auto& snapshot:f.cursor) {
   if(it==c.m_inv.cursor_cend()||!snapshot->Matches(EQ::invslot::slotCursor,*it))return false;
   ++it;
  }
 }
 return true;
}
bool NativeItemCustody::Click(Client& c,int16_t slot,const EQ::ItemInstance* actual,
 uint32_t target,bool supported,ClickTicket& ticket) {
 if(ticket.fence_)return false;
 if(!actual||!OriginalClickItem(actual->GetID()))return true;
 auto* owner=Owned(c);
 if(!owner&&(FireSharedTransformSource::OriginalRule(actual->GetID())||actual->GetID()==47756)) {
  c.m_fire_item_custody.reset(new NativeItemCustody);owner=Owned(c);
 }
 if(!owner||!owner->Capture(c,slot,actual,target,supported))return false;
 ticket.fence_=owner->state_->click->packet;return true;
}
bool NativeItemCustody::Launch(Client& c,int16_t slot,int32_t spell,uint32_t target,const ClickTicket& ticket) {
 const auto* actual=c.m_inv.GetItem(slot);
 if(!ticket.fence_)return !actual||!OriginalClickItem(actual->GetID());
 auto* owner=Owned(c);if(!owner||!owner->state_->click)return false;
 auto& f=*owner->state_->click;
 if(f.packet!=ticket.fence_||f.sequence!=ticket.fence_->sequence||ticket.fence_->returned||
  f.phase!=State::Phase::Captured||spell!=f.native_click_spell||
  slot!=f.slot||target!=f.target||!owner->Revalidate(c))return false;
 f.phase=State::Phase::LaunchReady;return true;
}
bool NativeItemCustody::Start(Client& c,int16_t slot,int32_t spell,uint32_t target) {
 const auto* actual=c.m_inv.GetItem(slot);
 auto* owner=Owned(c);
 // An old completed/rejected Fire click must not claim a later ordinary item
 // in the reused slot (including Binding846). Active selected custody still
 // rejects a changed/missing original; ordinary paths never borrow its ticket.
 if(!actual||!OriginalClickItem(actual->GetID())) {
  const auto* selected=owner?owner->state_->click.get():nullptr;
  return !selected||selected->slot!=slot||!selected->fence.Active();
 }

 if(!owner||!owner->state_->click||owner->state_->click->slot!=slot)
  return !actual||!OriginalClickItem(actual->GetID());
 auto& f=*owner->state_->click;
 if(f.phase!=State::Phase::LaunchReady||slot!=f.slot||spell!=f.native_click_spell||target!=f.target||!owner->Revalidate(c))return false;
 f.fence.launched_=true;f.phase=State::Phase::Casting;return true;
}
bool NativeItemCustody::FinishEntry(Client& c,int16_t slot,int32_t spell,int16_t debit,FinishTicket& ticket) {
 if(ticket.Selected())return false;
 const auto* actual=c.m_inv.GetItem(slot);
 auto* owner=Owned(c);
 // An old completed/rejected Fire click must not claim a later ordinary item
 // in the reused slot (including Binding846). Active selected custody still
 // rejects a changed/missing original; ordinary paths never borrow its ticket.
 if(!actual||!OriginalClickItem(actual->GetID())) {
  const auto* selected=owner?owner->state_->click.get():nullptr;
  return !selected||selected->slot!=slot||!selected->fence.Active();
 }

 if(!owner||!owner->state_->click||owner->state_->click->slot!=slot) {
  return false;
 }
 auto& f=*owner->state_->click;
 if(f.slot!=slot||spell!=f.native_click_spell||f.phase!=State::Phase::Casting||debit!=f.slot||!owner->Revalidate(c))return false;
 f.debit_slot=debit;f.phase=State::Phase::Finishing;ticket.packet_=f.packet;ticket.session_=f.session;ticket.original_=&c;ticket.character_=c.CharacterID();return true;
}
thread_local NativeItemCustody::DeleteScope* NativeItemCustody::DeleteScope::current_=nullptr;
NativeItemCustody::DeleteScope::DeleteScope(const FinishTicket& ticket) noexcept
 :ticket_(ticket),previous_(current_){current_=this;}
NativeItemCustody::DeleteScope::~DeleteScope(){current_=previous_;}
bool NativeItemCustody::StillLive(const FinishTicket& ticket) noexcept {
 if(!ticket.Selected())return true;
 auto* live=entity_list.GetClientByCharID(ticket.character_);
 return live&&live==ticket.original_&&live->InventorySessionReleaseTicket()==ticket.session_&&
  ticket.session_&&ticket.session_->HeldFor(*live);
}
bool NativeItemCustody::Finished(FinishTicket& ticket,bool success) {
 if(!ticket.Selected())return true;
 if(!StillLive(ticket))return false;
 auto& c=*entity_list.GetClientByCharID(ticket.character_);
 auto* owner=Owned(c);if(!owner||!owner->state_->click)return false;
 auto& f=*owner->state_->click;
 if(f.packet!=ticket.packet_||f.phase!=State::Phase::Finishing)return false;
 f.phase=success?State::Phase::SpellSucceeded:State::Phase::Rejected;
 if(!success)owner->state_->click.reset();
 return true;
}
void NativeItemCustody::Cancel(Client& c) {
 auto* owner=Owned(c);if(!owner||!owner->state_->click)return;
 const auto phase=owner->state_->click->phase;
 // CastSpell clears old native variables before accepting a LaunchReady cast.
 // Unknown custody must survive cancellation and is not a retry opportunity.
 if(phase==State::Phase::Casting||phase==State::Phase::Finishing||
  phase==State::Phase::Rejected)owner->state_->click.reset();
}
bool NativeItemCustody::DeleteSelected(Client& c,int16_t slot,int16_t quantity,bool client_update,bool update_db) {
 auto* scope=DeleteScope::current_;
 if(!scope||scope->entered_||!scope->ticket_.Selected())return false; // exact ordinary non-Fire path
 scope->entered_=true; // SAME one invocation; callbacks cannot borrow its authority
 const auto& ticket=scope->ticket_;
 // A selected native invocation must never fall through to the old live debit,
 // even if a mutable spell callback canceled/replaced the owned cast context.
 if(ticket.original_!=&c||!StillLive(ticket))return true;
 auto* owner=Owned(c);if(!owner||!owner->state_->click)return true;
 auto& f=*owner->state_->click;
 if(f.packet!=ticket.packet_||f.phase!=State::Phase::SpellSucceeded||
  f.slot!=slot||f.debit_slot!=slot||quantity!=1||!update_db||unsigned(bool(f.original_use))+unsigned(bool(f.shared_use))+unsigned(bool(f.helper_use))!=1)return true;
 PublishSelected(c,client_update);return true; // selected calls never reach ordinary debit
}
FireRunActionSource::DoorActionResult NativeItemCustody::DoorSelected(Client& c,
 std::unique_ptr<FireRunActionSource::OriginalDoorUse> original,
 std::unique_ptr<FireRunActionSource::PreparedRunDoorPlan> plan) {
 using namespace FireRunActionSource;
 const auto rejected=DoorActionResult::SelectedRejected;
 if(!original||!plan||!zone||!c.task_state||!c.CharacterID()||!c.GetID()||
  !NativeRunActionCustody::RevalidateDoorPlan(c,*plan))return rejected;
 const auto facts=original->Facts();
 if(!facts.native_sequence||facts.character!=c.CharacterID()||facts.client_entity!=c.GetID()||
  facts.zone!=zone->GetZoneID()||facts.instance!=zone->GetInstanceID()||
  !facts.door_database_id||!facts.native_door_id)return rejected;
 const bool world=facts.kind==RunActionKind::WorldFontKey;
 if(world!=!facts.mission)return rejected;
 if(!c.m_fire_item_custody)c.m_fire_item_custody.reset(new NativeItemCustody);
 auto* owner=Owned(c);
 if(owner->state_->click&&owner->state_->click->phase!=State::Phase::Rejected&&
  owner->state_->click->phase!=State::Phase::Committed)return rejected;
 auto selected=std::make_unique<State::Click>(std::nullopt,std::make_shared<PacketFence>(),facts.native_sequence,
  c.GetID(),0,zone->GetZoneID(),zone->GetInstanceID(),-1);
 selected->door_use=std::move(original);selected->door_plan=std::move(plan);
 selected->packet->sequence=facts.native_sequence;selected->phase=State::Phase::DoorReady;
 selected->inventory_mutates=false;
 if(!world) {
  selected->latch=NativeRunActionCustody::PreviewActual(*selected->door_use);
  if(!selected->latch)return rejected;
  selected->inventory_mutates=selected->latch->debit;
 }
 selected->session=c.InventorySessionReleaseTicket();
 selected->lease=selected->session?selected->session->SnapshotFor(c):std::nullopt;
 if(!selected->lease||!c.HasInventorySessionReservation())return rejected;
 const bool needs_physical=world||!selected->latch->row;
 if(needs_physical) {
  const uint32_t first=world?47759:facts.kind==RunActionKind::InnerVault?47748:
   facts.kind==RunActionKind::WorshipPair?47762:47757;
  if(world&&(facts.required_count!=1||facts.required_item_ids[0]!=47759||facts.physical_count!=1))return rejected;
  const PhysicalItemFacts* primary=nullptr;
  for(size_t i=0;i<facts.physical_count&&i<facts.physical_items.size();++i)
   if(facts.physical_items[i].item_id==first){if(primary)return rejected;primary=&facts.physical_items[i];}
  if(!primary||primary->units!=1)return rejected;
  selected->slot=primary->slot;selected->debit_slot=primary->slot;
  for(size_t i=0;i<facts.physical_count&&i<facts.physical_items.size();++i) {
   const auto& item=facts.physical_items[i];
   if(item.units!=1||(!CarriedGeneral(c.m_inv,item.slot)&&item.slot!=EQ::invslot::slotCursor))return rejected;
   const auto* actual=c.m_inv.GetItem(item.slot);
   if(!actual||!actual->GetItem()||actual->GetID()!=item.item_id||actual->GetSerialNumber()!=item.serial||
    actual->GetCharges()!=item.charges||!actual->IsClassCommon()||actual->IsStackable()||!actual->GetContents()->empty())return rejected;
   const auto root=RootSlot(item.slot);
   if(root==EQ::invslot::slotCursor)selected->cursor_participates=true;
   else if(!selected->roots.count(root)) {
    auto snapshot=OriginalItemSnapshot::Capture(root,c.m_inv.GetItem(root));if(!snapshot)return rejected;
    selected->roots.emplace(root,std::move(snapshot));
   }
   if(item.slot!=primary->slot)selected->b.push_back({item.slot,1,actual,item.item_id});
  }
  selected->clicked=OriginalItemSnapshot::Capture(primary->slot,c.m_inv.GetItem(primary->slot));
  if(!selected->clicked)return rejected;
  if(!world) {
   selected->action_origins=NativeRunItemOrigins::PrepareActionActual(c,selected->door_use.get(),nullptr);
   if(!selected->action_origins)return rejected;
  }
 }
 if(selected->cursor_participates)for(auto it=c.m_inv.cursor_cbegin();it!=c.m_inv.cursor_cend();++it) {
  auto snapshot=OriginalItemSnapshot::Capture(EQ::invslot::slotCursor,*it);if(!snapshot)return rejected;
  selected->cursor.push_back(std::move(snapshot));
 }
 selected->fence.inventory_=&c.m_inv;selected->fence.character_=c.CharacterID();
 selected->fence.cursor_=selected->cursor_participates;selected->fence.launched_=true;
 const auto protect=[&](const auto& self,const EQ::ItemInstance* item,unsigned depth)->bool {
  if(!item||item->IsEvolving()||item->GetEvolveUniqueID()||item->GetHousingHouseID()||item->GetHousingCrateID()||
   (depth&&item->IsClassBag()))return false;
  selected->fence.instances_.push_back(item);
  for(const auto& child:*item->GetContents())if(child.second&&!self(self,child.second,depth+1))return false;
  return true;
 };
 for(const auto& root:selected->roots) {
  selected->fence.roots_.push_back(root.first);
  if(!protect(protect,c.m_inv.GetItem(root.first),0))return rejected;
 }
 if(selected->cursor_participates)for(auto it=c.m_inv.cursor_cbegin();it!=c.m_inv.cursor_cend();++it)
  if((*it)->IsClassBag()||!(*it)->GetContents()->empty()||!protect(protect,*it,0))return rejected;
 owner->state_->click=std::move(selected);owner->state_->click->fence.Attach();
 // No fake spell, FinishTicket, personal binding or callback continuation.
 // PublishSelected's final sink owns the intent/plan. Never access c/owner again.
 return PublishSelected(c,true)?DoorActionResult::CommittedToNativePublication:rejected;
}

bool NativeItemCustody::PublishSelected(Client& c,bool client_update) {
 auto* owner=Owned(c);if(!owner||!owner->state_->click)return false;
 auto& f=*owner->state_->click;const auto slot=f.slot;
 const auto session_ticket=f.session;const auto character_id=c.CharacterID();const Client* expected=&c;
 const auto live=[session_ticket,character_id,expected]()noexcept {
  auto* current=entity_list.GetClientByCharID(character_id);
  return current&&current==expected&&session_ticket&&current->InventorySessionReleaseTicket()==session_ticket&&session_ticket->HeldFor(*current);
 };
 if(!live()||!owner->Revalidate(c)){f.phase=State::Phase::Rejected;f.fence.Detach();return false;}
 bool transaction=false;
 bool known_commit=false;
 std::unique_ptr<PreparedConstructPlan> prepared_plan;
 std::optional<PublishedNativeUse> handoff;
 std::optional<FireSharedTransformSource::PublishedSharedTransformUse> shared_handoff;
 std::optional<FireRunActionSource::PublishedRunActionReceipt> run_handoff;
 std::unique_ptr<FireRunActionSource::PublishedRunActionReceipt::State> run_receipt;
 try {
  // SAME successful original cast and original selected debit invocation only.
  // Preparation owns the actual staged actor/definition/attempt/eligible roster;
  // it must not register actors, spend, credit, or pump mutable callbacks.
  if(f.context&&f.context->consumer==Consumer::ActivateConstruct) {
   prepared_plan=PreparedConstructPlan::PrepareActual(c,*f.original_use,f.sequence);
   if(!prepared_plan||!prepared_plan->RevalidateBeforePersist(c))
    throw std::runtime_error("Fire actual construct plan unavailable before spend");
  }
  if(f.helper_use) {
   f.helper_plan=FireRunActionSource::PreparedRunHelperPlan::PrepareActual(c,*f.helper_use,f.sequence);
   if(!f.helper_plan||!f.helper_plan->RevalidateBeforePersist(c))throw std::runtime_error("Fire helper scene unavailable before debit");
  }
  if(f.helper_use||f.door_use) {
   run_receipt=std::make_unique<FireRunActionSource::PublishedRunActionReceipt::State>();run_receipt->owner=&c;
  }
  f.staged=std::make_unique<EQ::InventoryProfile>();
  f.staged->SetInventoryVersion(c.m_inv.InventoryVersion());
  f.staged->SetGMInventory(c.m_inv.GMInventory());
  for(const auto& root:f.roots) {
   auto value=root.second->CloneValue();if(!value)throw std::runtime_error("Fire root clone failed");
   if(f.staged->PushItem(root.first,value.get())!=root.first)throw std::runtime_error("Fire staged root failed");
   value.release();
  }
  if(f.cursor_participates)for(const auto& snapshot:f.cursor) {
   auto value=snapshot->CloneValue();if(!value)throw std::runtime_error("Fire cursor clone failed");
   f.staged->m_cursor.push(value.get());value.release();
  }
  // These mutations touch only owned off-profile values, never selected live A/B.
  if(f.inventory_mutates&&!f.staged->DeleteItem(slot,1))throw std::runtime_error("Fire staged A not removed");
  if(f.inventory_mutates)for(const auto& take:f.b) {
   auto* before=f.staged->GetItem(take.slot);
   if(!before||before->GetID()!=take.item_id||
    (before->IsStackable()&&before->GetCharges()<take.quantity))
    throw std::runtime_error("Fire staged B changed");
   f.staged->DeleteItem(take.slot,take.quantity);
  }
  uint32_t output_item=0;int32_t output_serial=0;int16_t output_charges=0;const EQ::ItemInstance* actual_output=nullptr;
  if(f.shared_use||(f.context&&f.context->consumer==Consumer::CombineIngredients)) {
   if(!f.output)throw std::runtime_error("Fire canonical output absent");
   actual_output=f.output.get();output_item=f.output->GetID();output_serial=f.output->GetSerialNumber();output_charges=f.output->GetCharges();
   if(f.output_slot==EQ::invslot::slotCursor)f.staged->m_cursor.push(f.output.get());
   else if(f.staged->PushItem(f.output_slot,f.output.get())!=f.output_slot)
    throw std::runtime_error("Fire staged output destination failed");
   f.output.release();
  }
  // Reserve null root nodes and pin actual map-value addresses BEFORE SQL.
  // std::map insertion elsewhere cannot invalidate these selected node addresses.
  std::vector<std::pair<EQ::ItemInstance**,EQ::ItemInstance**>> exchanges;
  exchanges.reserve(f.roots.size());
  for(const auto& root:f.roots) {
   auto live=c.m_inv.m_inv.find(root.first);
   if(live==c.m_inv.m_inv.end())throw std::runtime_error("Fire live root node disappeared");
   auto staged=f.staged->m_inv.emplace(root.first,nullptr).first;
   exchanges.emplace_back(&live->second,&staged->second);
  }
  // Prepare only the native selected-slot deltas, not a replay of the queue.
  // Serialize retains pointers to staged instances that become the SAME live
  // instances at publication. No cloned packet-only output identity is minted.
  f.packets.reserve(f.b.size()+3);
  const auto packet=[&](int16_t changed,const EQ::ItemInstance* item,ItemPacketType type) {
   if(item) {
    const auto bytes=item->Serialize(changed);
    auto p=std::make_unique<EQApplicationPacket>(OP_ItemPacket,bytes.size()+sizeof(ItemPacket_Struct));
    auto* wire=reinterpret_cast<ItemPacket_Struct*>(p->pBuffer);wire->PacketType=type;
    std::memcpy(wire->SerializedItem,bytes.data(),bytes.size());f.packets.push_back(std::move(p));
   }else {
    auto p=std::make_unique<EQApplicationPacket>(OP_MoveItem,sizeof(MoveItem_Struct));
    auto* wire=reinterpret_cast<MoveItem_Struct*>(p->pBuffer);
    wire->from_slot=changed;wire->to_slot=0xFFFFFFFF;wire->number_in_stack=0xFFFFFFFF;
    f.packets.push_back(std::move(p));
   }
  };
  if(client_update&&f.inventory_mutates) {
   packet(slot,slot==EQ::invslot::slotCursor?nullptr:f.staged->GetItem(slot),ItemPacketTrade);
   for(const auto& take:f.b)packet(take.slot,f.staged->GetItem(take.slot),ItemPacketTrade);
   if(output_item&&f.output_slot==EQ::invslot::slotCursor) {
    auto it=f.staged->m_cursor.end();--it;packet(EQ::invslot::slotCursor,*it,ItemPacketLimbo);
   }
  }
  f.save_results.assign(f.roots.size()+(f.cursor_participates?1:0),0);
  const auto original=f.original_use;const auto shared_original=f.shared_use;const auto sequence=f.sequence;
  const auto output_slot=f.output_slot;
  std::array<uint32_t,6> achievement_items{};
  if(shared_original) {
   for(size_t i=0;i<shared_original->shared.rule.input_count;++i)
    achievement_items[i]=shared_original->shared.rule.inputs[i];
   achievement_items[5]=output_item;
  }else if(original) {
   achievement_items[0]=original->item_id;
   achievement_items[1]=f.context->consumer==Consumer::CombineIngredients?f.context->rule.ingredient_b:0;
   achievement_items[2]=output_item;
  }
  // No callbacks/sends/event pumping in the following transaction/publication.
  const auto query=[&](const std::string& sql) {
   if(database.GetStatus()!=DBcore::Connected)throw std::runtime_error("Fire disconnected");
   auto result=database.QueryDatabase(sql,false);
   if(!result.Success())throw std::runtime_error("Fire strict native query failed");
   return result;
  };
  const auto number=[](const char* value)->uint64_t {
   if(!value)throw std::runtime_error("Fire null native field");return std::stoull(value);
  };
  const auto hex=[](const uint8_t* value,size_t size) {
   const char* digits="0123456789ABCDEF";std::string out;out.reserve(size*2);
   for(size_t i=0;i<size;++i){out+=digits[value[i]>>4];out+=digits[value[i]&15];}return out;
  };
  auto session=query("SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(session.RowCount()!=1||number(session.begin()[1])||number(session.begin()[2])!=1)
   throw std::runtime_error("Fire writer requires a fresh native transaction");
  const auto connection=number(session.begin()[0]);
  if(!database.TransactionBeginStrict().Success())throw std::runtime_error("Fire strict begin failed");
  transaction=true;
  const auto& lease=*f.lease;const auto actor=std::to_string(c.CharacterID());
  auto engines=query("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_data','housing_inventory_sessions','housing_live_item_leases')");
  if(engines.RowCount()!=1||number(engines.begin()[0])!=4)throw std::runtime_error("Fire native foundation differs");
  auto character=query("SELECT id FROM character_data WHERE id="+actor+" AND account_id="+std::to_string(c.AccountID())+" AND deleted_at IS NULL FOR UPDATE");
  if(character.RowCount()!=1)throw std::runtime_error("Fire native actor changed");
  auto held=query("SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+actor+" FOR UPDATE");
  if(held.RowCount()!=1)throw std::runtime_error("Fire inventory session absent");
  auto row=held.begin();
  if(!lease.active||number(row[0])!=lease.actor.account_id||number(row[1])!=lease.epoch||!row[2]||
   std::string(row[2])!=hex(lease.token.data(),lease.token.size())||!row[3]||
   std::string(row[3])!=hex(reinterpret_cast<const uint8_t*>(lease.owner.process_key.data()),lease.owner.process_key.size())||!row[4]||
   std::string(row[4])!=hex(reinterpret_cast<const uint8_t*>(lease.owner.client_key.data()),lease.owner.client_key.size())||
   number(row[5])!=lease.owner.zone_id||number(row[6])!=lease.owner.instance_id||number(row[7])!=1)
   throw std::runtime_error("Fire inventory lease differs");
  auto custody=query("SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id="+actor+" FOR UPDATE");
  if(custody.RowCount()>1||(custody.RowCount()&&
   (number(custody.begin()[0])!=c.AccountID()||(custody.begin()[1]&&*custody.begin()[1]))))
   throw std::runtime_error("Fire inventory has pending custody");
  // Lock and compare ONLY the selected native durable rows. Existing projection
  // semantics supply NULL/custom data/color/ornament/augment checks; no new codec.
  std::string where;
  const auto clause=[&](const std::string& term){if(!where.empty())where+=" OR ";where+=term;};
  for(const auto& root:f.roots) {
   clause("slot_id="+std::to_string(root.first));
   const auto begin=EQ::InventoryProfile::CalcSlotId(root.first,uint8_t(0));
   if(begin!=INVALID_INDEX)clause("slot_id BETWEEN "+std::to_string(begin)+" AND "+std::to_string(begin+EQ::invbag::SLOT_COUNT-1));
  }
  if(f.cursor_participates){clause("slot_id="+std::to_string(EQ::invslot::slotCursor));clause("slot_id BETWEEN "+std::to_string(EQ::invbag::CURSOR_BAG_BEGIN)+" AND "+std::to_string(EQ::invbag::CURSOR_BAG_END));}
  if(where.empty())where="0";
  auto persisted=query("SELECT "+std::string(Housing::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+actor+" AND ("+where+") ORDER BY slot_id FOR UPDATE");
  std::map<int16_t,Housing::LiveItems::InventoryRow> roots;
  std::vector<Housing::LiveItems::InventoryRow> children;
  std::map<int16_t,Housing::LiveItems::InventoryRow> cursor;
  for(auto sqlrow=persisted.begin();sqlrow!=persisted.end();++sqlrow) {
   auto value=Housing::InventoryRowDB::Decode(sqlrow);const auto physical=int16_t(value.values.slot_id);
   if(f.cursor_participates&&(physical==EQ::invslot::slotCursor||EQ::ValueWithin(physical,EQ::invbag::CURSOR_BAG_BEGIN,EQ::invbag::CURSOR_BAG_END)))cursor.emplace(physical,std::move(value));
   else if(f.roots.count(physical))roots.emplace(physical,std::move(value));
   else children.push_back(std::move(value));
  }
  for(auto& child:children) {
   const auto parent=EQ::InventoryProfile::CalcSlotId(int16_t(child.values.slot_id));auto at=roots.find(parent);
   if(at==roots.end())throw std::runtime_error("Fire native orphan child");
   child.values.slot_id=EQ::InventoryProfile::CalcBagIdx(int16_t(child.values.slot_id));at->second.children.push_back(std::move(child));
  }
  using Scope=Housing::InventoryProjection::ReconstructionScope;
  if(roots.size()!=f.roots.size())throw std::runtime_error("Fire native root universe differs");
  for(auto& root:roots)if(!Housing::InventoryProjection::Matches(root.second,*c.m_inv.GetItem(root.first),Scope::CharacterProfile))
   throw std::runtime_error("Fire native root preimage differs");
  if(f.cursor_participates) {
   if(cursor.size()!=f.cursor.size())throw std::runtime_error("Fire native cursor universe differs");
   auto it=c.m_inv.cursor_cbegin();
   for(size_t i=0;i<f.cursor.size();++i,++it) {
    const auto physical=i==0?EQ::invslot::slotCursor:int16_t(EQ::invbag::CURSOR_BAG_BEGIN+i);
    auto at=cursor.find(physical);
    if(at==cursor.end()||!Housing::InventoryProjection::Matches(at->second,**it,Scope::CharacterProfile))
     throw std::runtime_error("Fire native cursor preimage differs");
   }
  }
  if(!owner->Revalidate(c))throw std::runtime_error("Fire selected frame changed before save");
  if(prepared_plan&&!prepared_plan->RevalidateBeforePersist(c))
   throw std::runtime_error("Fire construct plan changed before persistence");
  if(f.helper_plan&&!f.helper_plan->RevalidateBeforePersist(c))throw std::runtime_error("Fire helper plan changed before persistence");
  uint64_t action_row=0;
  if(f.door_use||f.helper_use) {
   if(f.latch)action_row=FireRunActionSource::NativeRunActionCustody::StageActual(database,connection,*f.latch,
    f.door_use.get(),f.helper_use.get(),f.action_origins.get(),sequence,f.persistence_entered);
   else if(!f.door_use||f.door_use->Facts().kind!=FireRunActionSource::RunActionKind::WorldFontKey)
    throw std::runtime_error("Fire selected run latch unavailable");
  }
  if(f.transform_origins)FireRunActionSource::NativeRunItemOrigins::StageTransformActual(database,connection,*f.transform_origins,f.persistence_entered);
  f.fence.writer_=&database;
  size_t step=0;
  if(f.inventory_mutates)for(const auto& root:f.roots) {
   if(f.helper_plan&&!f.helper_plan->RevalidateBeforePersist(c))throw std::runtime_error("Fire helper plan changed before root save");
   if(f.door_plan&&!FireRunActionSource::NativeRunActionCustody::RevalidateDoorPlan(c,*f.door_plan))throw std::runtime_error("Fire door plan changed before root save");
   if(prepared_plan&&!prepared_plan->RevalidateBeforePersist(c))
    throw std::runtime_error("Fire construct plan changed before selected root save");
   f.persistence_entered=true;
   const bool saved=database.SaveInventory(c.CharacterID(),f.staged->GetItem(root.first),root.first);
   f.save_results[step++]=saved?1:2;if(!saved)throw std::runtime_error("Fire selected root save failed");
  }
  if(f.inventory_mutates&&f.cursor_participates) {
   if(f.helper_plan&&!f.helper_plan->RevalidateBeforePersist(c))throw std::runtime_error("Fire helper plan changed before cursor save");
   if(f.door_plan&&!FireRunActionSource::NativeRunActionCustody::RevalidateDoorPlan(c,*f.door_plan))throw std::runtime_error("Fire door plan changed before cursor save");
   auto begin=f.staged->cursor_cbegin(),end=f.staged->cursor_cend();
   if(prepared_plan&&!prepared_plan->RevalidateBeforePersist(c))
    throw std::runtime_error("Fire construct plan changed before selected cursor save");
   f.persistence_entered=true;
   const bool saved=database.SaveCursor(c.CharacterID(),begin,end);
   f.save_results[step++]=saved?1:2;if(!saved)throw std::runtime_error("Fire whole cursor save failed");
  }
  // Origin rows stay published1 and locked through the final plan/root/cursor
  // revalidation. Consume them AFTER selected saves, still BEFORE this SAME
  // strict COMMIT. Scene validators must not mistake our staged state2 for a
  // foreign/unpublished input while its actual native tree is still pre-swap.
  if(f.action_origins)FireRunActionSource::NativeRunItemOrigins::ConsumeActionActual(database,connection,*f.action_origins,action_row);
  f.fence.writer_=nullptr;
  auto final_session=query("SELECT CONNECTION_ID(),@@in_transaction");
  if(final_session.RowCount()!=1||number(final_session.begin()[0])!=connection||number(final_session.begin()[1])!=1||database.TransactionStrictFailed())
   throw std::runtime_error("Fire strict session changed");
  f.commit_attempted=true;
  const auto commit=database.TransactionCommitStrict();transaction=false;
  if(!commit.Success())throw std::runtime_error("Fire COMMIT not known successful");
  known_commit=true;
  // NO ALLOCATION, callbacks, sends, SQL, lookup or fallible native mutation.
  // The preallocated map values and complete list change ownership by noexcept
  // swaps only. Old original trees stay owned in f.staged until normal teardown.
  if(f.inventory_mutates) {
   for(const auto& exchange:exchanges)std::swap(*exchange.first,*exchange.second);
   if(f.cursor_participates)c.m_inv.m_cursor.Swap(f.staged->m_cursor);
  }
  f.phase=State::Phase::Committed;f.fence.Detach();
  // Private constructor runs here, before any mutable achievement callback.
  // Moving into stack optional is noexcept and revokes the temporary authority.
  if(f.transform_origins&&(!actual_output||!FireRunActionSource::NativeRunItemOrigins::PublishTransformActual(c,*f.transform_origins,*actual_output,output_slot))) {
   f.phase=State::Phase::Unknown;
   throw std::runtime_error("Fire committed output origin publication unknown; terminal hold");
  }
  if(run_receipt) {
   const auto kind=f.helper_use?FireRunActionSource::RunActionKind::HelperActivation:f.door_use->Facts().kind;
   const auto boundary=f.latch?f.latch->boundary:FireRunActionSource::RunActionBoundary::RetainedWorldPhysicalKey;
   const auto run=f.latch?f.latch->run:0;
   run_receipt->door_original=std::move(f.door_use);run_receipt->helper_original=std::move(f.helper_use);
   run_receipt->door_plan=std::move(f.door_plan);run_receipt->helper_plan=std::move(f.helper_plan);
   run_receipt->view={kind,boundary,action_row,run,sequence,run_receipt->door_original.get(),run_receipt->helper_original.get(),run_receipt->door_plan.get(),run_receipt->helper_plan.get()};
   FireRunActionSource::PublishedRunActionReceipt issued(std::move(run_receipt));run_handoff.emplace(std::move(issued));
  }else if(shared_original) {
   FireSharedTransformSource::PublishedSharedTransformUse issued(c,*shared_original,
    sequence,output_item,output_serial,output_slot,output_charges);
   shared_handoff.emplace(std::move(issued));
  }else {
   PublishedNativeUse issued(c,*original,sequence,output_item,output_serial,
    output_slot,output_charges,std::move(prepared_plan));
   handoff.emplace(std::move(issued));
  }
  for(auto& prepared:f.packets) {
   auto* send=prepared.release();c.FastQueuePacket(&send);safe_delete(send);
  }
  // Beyond the first mutable callback use only stack values and the existing
  // independent native session ticket/registry lifetime check, never f/owner.
  for(const auto item:achievement_items)if(item) {
   c.UpdateAchievementForOwnItem(item);
   if(!live())return true;
  }
 }catch(...) {
  if(known_commit) {
   // Inventory memory publication is allocation-free. Origin acknowledgment
   // or a later packet failure never replays a committed debit/output/receipt.
   if(live())c.RejectInventorySessionAdmission();
   return false;
  }
  f.fence.writer_=nullptr;
  if(transaction)database.TransactionRollbackStrict();
  if(f.persistence_entered||f.commit_attempted) {
   f.phase=State::Phase::Unknown;
   c.RejectInventorySessionAdmission(); // existing lease retained to full delete
  }else {f.phase=State::Phase::Rejected;f.fence.Detach();}
  return false;
 }
 // Deliberately outside catch: even an exception from the mutable sink must
 // never send control back into code that dereferences Client or held trees.
 if(run_handoff) {
  FireRunActionSource::OnOriginalRunActionPublished(c,*run_handoff);return true;
 }
 if(shared_handoff) {
  FireSharedTransformSource::OnOriginalSharedTransformPublished(c,*shared_handoff);
  return true;
 }
 OnOriginalNativeUsePublished(c,*handoff);
 return true;
}

FireRunActionSource::OriginalTerminalReturnResult NativeItemCustody::MoldReturnSelected(Client& c,
 std::unique_ptr<FireRunActionSource::OriginalMoldTrade> original,
 std::unique_ptr<FireRunActionSource::PreparedTerminalChestPlan> plan) {
 using namespace FireRunActionSource;
 using Result=OriginalTerminalReturnResult;
 if(!original||!plan||!zone||!c.HasInventorySessionReservation()||
  !NativeRunTerminalCustody::ValidateOriginalMoldReturn(c,*original)||!plan->RevalidateBeforePersist(c))return Result::RejectedBeforeMutation;
 if(!c.m_fire_item_custody)c.m_fire_item_custody.reset(new NativeItemCustody);
 auto* owner=Owned(c);
 if((owner->state_->click&&owner->state_->click->fence.Active())||
  (owner->state_->mold_return&&owner->state_->mold_return->phase==State::MoldPhase::Unknown))return Result::RejectedBeforeMutation;
 auto pending=std::make_unique<State::MoldReturn>();
 pending->receipt=std::make_unique<PublishedTerminalReturnReceipt::State>();pending->receipt->owner=&c;
 pending->receipt->original=std::move(original);pending->receipt->plan=std::move(plan);
 const auto& facts=pending->receipt->original->Facts();const auto slot=facts.mold.slot;
 pending->original=c.m_inv.GetItem(slot);pending->mold=OriginalItemSnapshot::Capture(slot,pending->original);
 if(!pending->mold)return Result::RejectedBeforeMutation;
 pending->token=ItemOriginToken(*pending->original);
 pending->session=c.InventorySessionReleaseTicket();pending->lease=pending->session?pending->session->SnapshotFor(c):std::nullopt;
 if(!pending->lease||!pending->session->HeldFor(c))return Result::RejectedBeforeMutation;
 pending->fence.inventory_=&c.m_inv;pending->fence.character_=c.CharacterID();pending->fence.launched_=true;
 for(int16_t at=EQ::invslot::TRADE_BEGIN;at<=EQ::invslot::TRADE_NPC_END;++at)pending->fence.roots_.push_back(at);
 pending->fence.instances_.push_back(pending->original);
 owner->state_->mold_return=std::move(pending);auto& f=*owner->state_->mold_return;f.fence.Attach();
 const auto character_id=c.CharacterID();const Client* expected=&c;const auto ticket=f.session;
 const auto live=[character_id,expected,ticket]()noexcept {
  auto* now=entity_list.GetClientByCharID(character_id);
  return now&&now==expected&&ticket&&now->InventorySessionReleaseTicket()==ticket&&ticket->HeldFor(*now);
 };
 bool transaction=false,known_commit=false;
 std::optional<PublishedTerminalReturnReceipt> handoff;
 try {
  // The real trade map node is pinned before SQL. No ordinary PopItem, generic
  // delivery count, inventory mutation, trade reset or actor publication yet.
  auto node=c.m_inv.m_trade.find(slot);
  if(node==c.m_inv.m_trade.end()||node->second!=f.original||!live()||
   !f.mold->Matches(slot,c.m_inv.GetItem(slot))||!f.receipt->plan->RevalidateBeforePersist(c))
   throw std::runtime_error("Fire original mold trade changed before persistence");
  auto session=ActionQuery(database,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(session.RowCount()!=1||ActionNumber(session.begin()[1])||ActionNumber(session.begin()[2])!=1)
   throw std::runtime_error("Fire mold writer needs fresh strict session");
  const auto connection=ActionNumber(session.begin()[0]);
  if(!database.TransactionBeginStrict().Success())throw std::runtime_error("Fire mold strict begin failed");transaction=true;
  auto foundation=ActionQuery(database,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
   "AND table_name IN('inventory','character_data','housing_inventory_sessions','housing_live_item_leases')");
  if(foundation.RowCount()!=1||ActionNumber(foundation.begin()[0])!=4)throw std::runtime_error("Fire mold native inventory foundation differs");
  auto actor=ActionQuery(database,fmt::format("SELECT id FROM character_data WHERE id={} AND account_id={} AND deleted_at IS NULL FOR UPDATE",c.CharacterID(),c.AccountID()));
  if(actor.RowCount()!=1)throw std::runtime_error("Fire mold original actor changed");
  const auto hex=[](const uint8_t* data,size_t size) {
   const char* digits="0123456789ABCDEF";std::string value;value.reserve(2*size);
   for(size_t i=0;i<size;++i){value+=digits[data[i]>>4];value+=digits[data[i]&15];}return value;
  };
  const auto& lease=*f.lease;
  auto held=ActionQuery(database,fmt::format("SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active "
   "FROM housing_inventory_sessions WHERE character_id={} FOR UPDATE",c.CharacterID()));
  if(held.RowCount()!=1)throw std::runtime_error("Fire mold inventory lease absent");auto row=held.begin();
  if(!lease.active||ActionNumber(row[0])!=lease.actor.account_id||ActionNumber(row[1])!=lease.epoch||!row[2]||
   std::string(row[2])!=hex(lease.token.data(),lease.token.size())||!row[3]||
   std::string(row[3])!=hex(reinterpret_cast<const uint8_t*>(lease.owner.process_key.data()),lease.owner.process_key.size())||!row[4]||
   std::string(row[4])!=hex(reinterpret_cast<const uint8_t*>(lease.owner.client_key.data()),lease.owner.client_key.size())||
   ActionNumber(row[5])!=lease.owner.zone_id||ActionNumber(row[6])!=lease.owner.instance_id||ActionNumber(row[7])!=1)
   throw std::runtime_error("Fire mold native lease changed");
  auto custody=ActionQuery(database,fmt::format("SELECT account_id,pending_key FROM housing_live_item_leases WHERE character_id={} FOR UPDATE",c.CharacterID()));
  if(custody.RowCount()>1||(custody.RowCount()&&(ActionNumber(custody.begin()[0])!=c.AccountID()||
   (custody.begin()[1]&&*custody.begin()[1]))))throw std::runtime_error("Fire mold native pending custody exists");
  std::string where;
  for(int16_t at=EQ::invslot::TRADE_BEGIN;at<=EQ::invslot::TRADE_NPC_END;++at) {
   if(!where.empty())where+=" OR ";where+="slot_id="+std::to_string(at);
   const auto child=EQ::InventoryProfile::CalcSlotId(at,uint8_t(0));
   if(child!=INVALID_INDEX)where+=" OR slot_id BETWEEN "+std::to_string(child)+" AND "+std::to_string(child+EQ::invbag::SLOT_COUNT-1);
  }
  auto persisted=ActionQuery(database,"SELECT "+std::string(Housing::InventoryRowDB::Columns)+" FROM inventory WHERE character_id="+
   std::to_string(c.CharacterID())+" AND ("+where+") ORDER BY slot_id FOR UPDATE");
  if(persisted.RowCount()!=1)throw std::runtime_error("Fire mold durable four-root universe differs");
  auto durable=Housing::InventoryRowDB::Decode(persisted.begin());
  if(durable.values.slot_id!=slot||!Housing::InventoryProjection::Matches(durable,*f.original,
   Housing::InventoryProjection::ReconstructionScope::CharacterProfile))throw std::runtime_error("Fire mold original durable root differs");
  if(!live()||!f.mold->Matches(slot,c.m_inv.GetItem(slot))||!f.receipt->plan->RevalidateBeforePersist(c)||
   f.receipt->plan->AcceptedMembers()!=facts.accepted_members)throw std::runtime_error("Fire mold actual plan changed before selected write");
  const auto terminal=StageMoldReturnActual(database,connection,*f.receipt->original,f.token,f.persistence_entered);
  // Same locked origin remains published1 through all original Source plan
  // validation;consume2 only after the real selected save, before SAME COMMIT.
  if(!f.receipt->plan->RevalidateBeforePersist(c))throw std::runtime_error("Fire mold plan changed before native trade save");
  f.fence.writer_=&database;f.persistence_entered=true;
  const bool saved=database.SaveInventory(c.CharacterID(),nullptr,slot);
  if(!saved)throw std::runtime_error("Fire mold selected native trade save failed");
  ConsumeMoldOriginActual(database,connection,facts,f.token,terminal);f.fence.writer_=nullptr;
  ActionSession(database,connection);f.commit_attempted=true;
  const auto commit=database.TransactionCommitStrict();transaction=false;
  if(!commit.Success())throw std::runtime_error("Fire mold strict COMMIT unknown");known_commit=true;
  // Allocation-free same actual memory removal. The preexisting trade node is
  // retained;its original physical instance now belongs to this held frame.
  f.spent.reset(std::exchange(node->second,nullptr));f.phase=State::MoldPhase::Committed;f.fence.Detach();
  f.receipt->row=terminal;f.receipt->run=facts.mission.run;f.receipt->sequence=facts.native_sequence;
  PublishedTerminalReturnReceipt issued(std::move(f.receipt));handoff.emplace(std::move(issued));
 }catch(...) {
  if(known_commit){if(live())c.RejectInventorySessionAdmission();return Result::UnknownHold;}
  f.fence.writer_=nullptr;if(transaction)database.TransactionRollbackStrict();
  if(f.persistence_entered||f.commit_attempted){f.phase=State::MoldPhase::Unknown;c.RejectInventorySessionAdmission();return Result::UnknownHold;}
  f.phase=State::MoldPhase::Rejected;f.fence.Detach();return Result::RejectedBeforeMutation;
 }
 OnOriginalTerminalReturnPublished(c,*handoff); // final mutable sink, outside catch
 return Result::KnownCommitted; // stack only;never touch Client/state after sink
}

}
