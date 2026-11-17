#pragma once
#include "original_trade_recipe.h"

namespace EQEmu { namespace SharedCore { namespace WeightedPersonalDelivery {
namespace Trade = OriginalTradeRecipe;
// Spells supplies the actual active PERSONAL occurrence; task_id is never a key.
struct Occurrence {
 uint32_t character_id = 0;
 uint64_t occurrence_id = 0;
 uint32_t accepted_time = 0;
 uint32_t task_id = 0;
 uint32_t activity_id = 0;
 uint32_t progress = 0;
 uint32_t carry_pounds = 0;
 uint64_t carry_expires_at = 0;
 uint64_t revision = 0; // Persisted occurrence/carry CAS version, not a Lua hint.
};
struct Role { uint32_t item_id = 0; uint32_t pounds = 0; };
enum class StackRule { Unbound, ReturnStacked, AcceptStacked };
enum class CarryClock { Unbound, RefreshOnAccept, PreserveUnexpired };
enum class ExcessRule { Unbound, ConsumeEligibleAndDiscardAtCompletion };
struct Parameters {
 uint32_t task_id = 0;
 uint32_t activity_id = 0;
 uint32_t pounds_per_unit = 0;
 uint32_t goal_units = 0;
 std::array<Role, 3> roles{};
 StackRule stack_rule = StackRule::Unbound;
 ExcessRule excess_rule = ExcessRule::Unbound;
 CarryClock carry_clock = CarryClock::Unbound;
 uint64_t carry_lifetime_seconds = 0; // Zero means unbound, NOT infinite.
};
enum class Status { Ready, Invalid, Unbound, NoEligibleOffer, AlreadyComplete, ExcessPolicyUnbound };
struct Plan {
 Trade::DebitPlan debit;
 uint32_t accepted_pounds = 0;
 uint32_t credit_units = 0;
 uint32_t progress_after = 0;
 uint32_t carry_after = 0;
 uint64_t carry_expires_after = 0;
};
inline Status Prepare(const Parameters& p, const Occurrence& before, const Trade::Offers& offers,
                      uint64_t now, Plan& out) {
 out = {}; // Failed planning never leaves a debit candidate usable by caller.
 if (!p.task_id || !p.goal_units || !p.pounds_per_unit || !p.carry_lifetime_seconds || p.stack_rule == StackRule::Unbound || p.carry_clock == CarryClock::Unbound)
  return Status::Unbound;
 for (std::size_t i = 0; i < p.roles.size(); ++i) {
  if (!p.roles[i].item_id || !p.roles[i].pounds) return Status::Unbound;
  for (std::size_t j = 0; j < i; ++j) if (p.roles[i].item_id == p.roles[j].item_id) return Status::Invalid;
 }
 if (!before.character_id || (!before.occurrence_id || !before.accepted_time) || before.task_id != p.task_id || before.activity_id != p.activity_id ||
     before.carry_expires_at > UINT32_MAX || before.progress > p.goal_units || before.carry_pounds >= p.pounds_per_unit || !now ||
     (before.carry_pounds && !before.carry_expires_at) || (!before.carry_pounds && before.carry_expires_at) ||
     !Trade::Valid(offers, {})) return Status::Invalid;
 if (before.progress == p.goal_units) return Status::AlreadyComplete;
 Plan result;
 uint64_t accepted = 0;
 for (std::size_t i = 0; i < Trade::OfferCount; ++i) {
  const auto& o = offers[i]; if (!o.present) continue;
  for (const auto& role : p.roles) if (o.item_id == role.item_id) {
   const auto units = Trade::Available(o);
   if (o.stackable && units > 1 && p.stack_rule == StackRule::ReturnStacked) break;
   result.debit.units[i] = units;
   accepted += uint64_t(units) * role.pounds;
   break;
  }
 }
 if (!accepted) return Status::NoEligibleOffer;
 // Carry-expiry handling is locked into the same occurrence CAS as this debit.
 const uint64_t carry = before.carry_pounds && before.carry_expires_at > now ? before.carry_pounds : 0;
 const auto total = accepted + carry;
 const auto units = total / p.pounds_per_unit;
 const auto remaining = p.goal_units - before.progress;
 const auto remainder = total % p.pounds_per_unit;
 if ((units > remaining || (units == remaining && remainder)) && p.excess_rule == ExcessRule::Unbound)
  return Status::ExcessPolicyUnbound;
 if (accepted > UINT32_MAX || now > UINT32_MAX || p.carry_lifetime_seconds > UINT32_MAX - now) return Status::Invalid;
 result.accepted_pounds = uint32_t(accepted);
 result.credit_units = units > remaining ? remaining : uint32_t(units);
 result.progress_after = before.progress + result.credit_units;
 result.carry_after = result.progress_after == p.goal_units ? 0 : uint32_t(remainder);
 // Expiry refresh versus preservation is explicitly parameterized and unbound
 // by default. Neither behavior is asserted as historical source evidence.
 result.carry_expires_after = result.carry_after ?
  (p.carry_clock == CarryClock::PreserveUnexpired && carry ? before.carry_expires_at : now + p.carry_lifetime_seconds) : 0;
 if (!Trade::Valid(offers, result.debit)) return Status::Invalid;
 out = result; return Status::Ready;
}
}}}
