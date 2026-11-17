#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// Pure finite planning only. These values are NOT a custody/credit capability.
// Capture from the four server-owned NPC offer slots at ORIGINAL accept; the
// leased writer must still validate exact durable trees before any persistence.
namespace EQEmu { namespace SharedCore { namespace OriginalTradeRecipe {
constexpr std::size_t OfferCount = 4;
struct Offer {
 bool present = false;
 int16_t slot = -1;
 int32_t guid = 0;
 uint32_t item_id = 0;
 int16_t charges = 0;
 bool stackable = false;
};
using Offers = std::array<Offer, OfferCount>;
struct DebitPlan { std::array<uint32_t, OfferCount> units{}; };
inline uint32_t Available(const Offer& o) {
 return !o.present ? 0 : o.stackable ? (o.charges > 0 ? uint32_t(o.charges) : 0) : 1;
}
inline bool Valid(const Offers& offers, const DebitPlan& plan) {
 for (std::size_t i = 0; i < OfferCount; ++i) {
  const auto& o = offers[i];
  if (!o.present) { if (plan.units[i]) return false; continue; }
  if (o.slot < 0 || o.guid <= 0 || !o.item_id || !Available(o) || plan.units[i] > Available(o)) return false;
  for (std::size_t j = 0; j < i; ++j)
   if (offers[j].present && (offers[j].slot == o.slot || offers[j].guid == o.guid)) return false;
 }
 return true;
}
inline bool Same(const Offers& a, const Offers& b) {
 for (std::size_t i = 0; i < OfferCount; ++i)
  if (a[i].present != b[i].present || a[i].slot != b[i].slot || a[i].guid != b[i].guid ||
      a[i].item_id != b[i].item_id || a[i].charges != b[i].charges || a[i].stackable != b[i].stackable) return false;
 return true;
}
inline bool FirstUnit(const Offers& offers, uint32_t item_id, DebitPlan& out) {
 out = {};
 if (!item_id) return false;
 for (std::size_t i = 0; i < OfferCount; ++i) if (offers[i].present && offers[i].item_id == item_id) {
  DebitPlan candidate; candidate.units[i] = 1;
  if (!Valid(offers, candidate)) return false;
  out = candidate; return true;
 }
 return false;
}
// Shared by the existing writer's success-return staging. Returns all unrelated
// roots/charges; consumed nonstackable roots and exhausted stacks are omitted.
inline bool ReturnRoot(const Offer& offer, uint32_t debit) {
 return offer.present && debit < Available(offer);
}
inline int16_t ReturnedCharges(const Offer& offer, uint32_t debit) {
 return offer.stackable ? int16_t(int32_t(offer.charges) - int32_t(debit)) : offer.charges;
}
}}}
