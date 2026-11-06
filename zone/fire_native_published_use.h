#pragma once
// Feature-private SOURCE INTERFACE ONLY. No writer or callback is installed.
#include "fire_personal_item_use.h"
#include <cstdint>
#include <optional>
#include <memory>
#include <type_traits>
class Client;
namespace FirePersonalOccurrenceSource {
class NativeItemCustody;
class PreparedConstructPlan;
struct PublishedNativeUseView {
 OriginalItemUse original; // SAME pre-callback native item-use capture, by value
 std::uint64_t native_click_sequence;
 std::uint32_t published_output_item;
 std::int32_t published_output_serial;
 std::int16_t published_output_slot;
 std::int16_t published_output_charges;
 PreparedConstructPlan* prepared_construct_plan; // borrowed only while issuing receipt lives
};
static_assert(std::is_trivially_copyable<PublishedNativeUseView>::value,
 "The post-commit handoff must not allocate or clone native trees");
// Created ONLY by the selected custody writer after strict known COMMIT and
// allocation-free memory publication. Context/SpellFinished/SaveInventory alone
// MUST NEVER construct it. Failed/unknown persistence produces no such object.
class PublishedNativeUse final {
 friend class NativeItemCustody;
 const Client* owner_;
 std::unique_ptr<PreparedConstructPlan> prepared_plan_;
 PublishedNativeUseView view_;
 bool taken_=false;
 PublishedNativeUse(const Client& owner,const OriginalItemUse& original,
  std::uint64_t sequence,std::uint32_t output_item,std::int32_t output_serial,
  std::int16_t output_slot,std::int16_t output_charges,
  std::unique_ptr<PreparedConstructPlan> prepared_plan) noexcept;
public:
 PublishedNativeUse()=delete;
 PublishedNativeUse(const PublishedNativeUse&)=delete;
 PublishedNativeUse& operator=(const PublishedNativeUse&)=delete;
 PublishedNativeUse(PublishedNativeUse&&) noexcept;
 ~PublishedNativeUse();
 PublishedNativeUse& operator=(PublishedNativeUse&&)=delete;
 // Caller seals the one native producer attempt BEFORE any mutable callback,
 // allocation, spawn, counter increment or query. View copies are evidence for
 // that attempt, never authority to construct or replay another receipt.
 std::optional<PublishedNativeUseView> Take(const Client& actual_owner) noexcept {
  if(taken_ || owner_!=&actual_owner)return std::nullopt;
  taken_=true;
  return view_;
 }
};
// Spells can implement this exact native sink independently. The writer will
// invoke it once at its final publication boundary, then MUST NOT dereference
// Client/custody/selected objects after this mutable producer callback.
// CombineIngredients never spawns or credits activity2. ActivateConstruct may
// proceed only through the existing held placement/lifetime/cohort predicates.
void OnOriginalNativeUsePublished(Client& actual_owner,PublishedNativeUse& receipt);
} // namespace FirePersonalOccurrenceSource
