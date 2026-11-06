#pragma once
// Feature-private Fire custody component. The native caller must capture before
// mutable callbacks and revalidate before staging. This is not a debit receipt.
#include "../common/item_instance.h"
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <tuple>
#include <type_traits>

namespace FirePersonalOccurrenceSource {
class NativeItemCustody;
class OriginalItemSnapshot final {
 friend class NativeItemCustody;
 struct Node {
  const EQ::ItemInstance* original = nullptr; // comparison only, never dereferenced
  std::unique_ptr<EQ::ItemInstance> value;
  std::array<unsigned char,sizeof(EQ::ItemData)> definition{};
  std::array<unsigned char,sizeof(EQ::ItemData)> scaled_definition{};
  std::map<uint8,std::unique_ptr<Node>> children;
 };
 int16 slot_;
 std::unique_ptr<Node> root_;
 OriginalItemSnapshot(int16 slot,std::unique_ptr<Node> root)
  :slot_(slot),root_(std::move(root)){}
 OriginalItemSnapshot(const OriginalItemSnapshot&)=delete;
 OriginalItemSnapshot& operator=(const OriginalItemSnapshot&)=delete;

 static bool SameTimer(const Timer& a,const Timer& b) noexcept {
  return std::tie(a.start_time,a.timer_time,a.enabled,a.set_at_trigger,a.pUseAcurateTiming)==
   std::tie(b.start_time,b.timer_time,b.enabled,b.set_at_trigger,b.pUseAcurateTiming);
 }
 static bool SameValues(const EQ::ItemInstance& a,const EQ::ItemInstance& b) noexcept {
  const auto values=[](const EQ::ItemInstance& i) {
   const auto& e=i.m_evolving_details;
   return std::tie(i.m_use_type,i.m_charges,i.m_price,i.m_color,i.m_merchantslot,
    i.m_currentslot,i.m_attuned,i.m_merchantcount,i.m_SerialNumber,i.m_exp,
    i.m_evolveLvl,i.m_scaling,i.m_ornamenticon,i.m_ornamentidfile,
    i.m_new_id_file,i.m_ornament_hero_model,i.m_recast_timestamp,
    i.m_housing_estate_id,i.m_housing_house_id,i.m_housing_crate_id,
    i.m_task_delivered_count,e.id,e.character_id,e.item_id,e.activated,
    e.equipped,e.current_amount,e.progression,e.final_item_id,e.deleted_at);
  };
  if(values(a)!=values(b)||a.m_custom_data!=b.m_custom_data||
   a.m_timers.size()!=b.m_timers.size())return false;
  auto ai=a.m_timers.cbegin(),bi=b.m_timers.cbegin();
  for(;ai!=a.m_timers.cend();++ai,++bi)
   if(ai->first!=bi->first||!SameTimer(ai->second,bi->second))return false;
  return true;
 }
 // The existing native copy constructor omits m_task_delivered_count. Correct
 // only this feature's owned copy, recursively; do not change global Clone().
 static bool CompleteClone(EQ::ItemInstance& copy,const EQ::ItemInstance& source) {
  copy.m_task_delivered_count=source.m_task_delivered_count;
  // Clone normally omits null map entries. Preserve those keys as well.
  for(const auto& entry:source.m_contents) {
   auto found=copy.m_contents.find(entry.first);
   if(!entry.second) {
    if(found==copy.m_contents.end())copy.m_contents.emplace(entry.first,nullptr);
    else if(found->second)return false;
   }else if(found==copy.m_contents.end()||!found->second||
    !CompleteClone(*found->second,*entry.second))return false;
  }
  return copy.m_contents.size()==source.m_contents.size();
 }
 static std::unique_ptr<Node> CaptureNode(const EQ::ItemInstance* actual) {
  if(!actual)return {};
  static_assert(std::is_trivially_copyable<EQ::ItemData>::value,
   "Native definition snapshots require actual object representation");
  auto n=std::make_unique<Node>();n->original=actual;n->value.reset(actual->Clone());
  if(!n->value||!CompleteClone(*n->value,*actual))return {};
  if(actual->m_item)std::memcpy(n->definition.data(),actual->m_item,sizeof(EQ::ItemData));
  if(actual->m_scaledItem)std::memcpy(n->scaled_definition.data(),actual->m_scaledItem,sizeof(EQ::ItemData));
  for(const auto& entry:actual->m_contents) {
   auto child=CaptureNode(entry.second);
   if(entry.second&&!child)return {};
   n->children.emplace(entry.first,std::move(child));
  }
  return n;
 }
 static bool MatchesNode(const Node& n,const EQ::ItemInstance* live) noexcept {
  if(!live||live!=n.original||!n.value||!SameValues(*live,*n.value)||
   bool(live->m_item)!=bool(n.value->m_item)||
   bool(live->m_scaledItem)!=bool(n.value->m_scaledItem)||
   live->m_contents.size()!=n.children.size())return false;
  // Compare to bytes captured from the original, not padding reconstructed by
  // Clone. Changes to an original definition also invalidate this frame.
  if(live->m_item&&std::memcmp(n.definition.data(),live->m_item,sizeof(EQ::ItemData)))return false;
  if(live->m_scaledItem&&std::memcmp(n.scaled_definition.data(),live->m_scaledItem,sizeof(EQ::ItemData)))return false;
  auto it=live->m_contents.cbegin();
  for(const auto& entry:n.children) {
   if(it==live->m_contents.cend()||it->first!=entry.first)return false;
   if(entry.second) {if(!MatchesNode(*entry.second,it->second))return false;}
   else if(it->second)return false;
   ++it;
  }
  return true;
 }
 static std::unique_ptr<OriginalItemSnapshot> Capture(int16 slot,const EQ::ItemInstance* actual) {
  auto root=CaptureNode(actual);if(!root)return {};
  return std::unique_ptr<OriginalItemSnapshot>(new OriginalItemSnapshot(slot,std::move(root)));
 }
 bool Matches(int16 slot,const EQ::ItemInstance* live)const noexcept {
  return slot==slot_&&root_&&MatchesNode(*root_,live);
 }
 // Each returned clone is an owned value only. It never proves object identity.
 std::unique_ptr<EQ::ItemInstance> CloneValue()const {
  if(!root_||!root_->value)return {};
  std::unique_ptr<EQ::ItemInstance> copy(root_->value->Clone());
  if(!copy||!CompleteClone(*copy,*root_->value))return {};
  return copy;
 }
};
} // namespace FirePersonalOccurrenceSource
