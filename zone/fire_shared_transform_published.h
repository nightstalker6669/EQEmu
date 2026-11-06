#pragma once
#include "fire_shared_transform_use.h"
#include <optional>
#include <type_traits>
namespace FireSharedTransformSource {
struct PublishedSharedTransformView {
 OriginalSharedTransformUse original;
 uint64_t native_click_sequence;
 uint32_t published_output_item;
 int32_t published_output_serial;
 int16_t published_output_slot,published_output_charges;
};
static_assert(std::is_trivially_copyable<PublishedSharedTransformView>::value,
 "Shared transform publication must not allocate or clone inventory");
class PublishedSharedTransformUse final {
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 const Client* owner_;
 PublishedSharedTransformView view_;
 bool taken_=false;
 PublishedSharedTransformUse(const Client& owner,const OriginalSharedTransformUse& original,
  uint64_t sequence,uint32_t output,int32_t serial,int16_t slot,int16_t charges) noexcept
  :owner_(&owner),view_{original,sequence,output,serial,slot,charges} {}
public:
 PublishedSharedTransformUse()=delete;
 PublishedSharedTransformUse(const PublishedSharedTransformUse&)=delete;
 PublishedSharedTransformUse& operator=(const PublishedSharedTransformUse&)=delete;
 PublishedSharedTransformUse(PublishedSharedTransformUse&& other) noexcept
  :owner_(other.owner_),view_(other.view_),taken_(other.taken_) {
  other.owner_=nullptr;other.taken_=true;
 }
 PublishedSharedTransformUse& operator=(PublishedSharedTransformUse&&)=delete;
 std::optional<PublishedSharedTransformView> Take(const Client& actual_owner) noexcept {
  if(taken_||owner_!=&actual_owner)return std::nullopt;
  taken_=true;return view_;
 }
};
} // namespace FireSharedTransformSource
