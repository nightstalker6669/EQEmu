#pragma once

#include "dodh_rakban_claim.h"

#include <array>
#include <optional>

class Client;
namespace EQ { class ItemInstance; }
struct DBTradeskillRecipe_Struct;

namespace DodhRakbanCombine {
inline constexpr uint32_t ReservedRecipe = 30752007;
inline constexpr uint32_t Shroud = 85575;
inline constexpr uint32_t Skull = 85571;
inline constexpr uint32_t FinalSkull = 85573;

inline bool Selected(uint32_t recipe) { return recipe == ReservedRecipe; }

// The reservation identifies the protected route even while the owner binding
// is disabled. An accidentally enabled content row cannot use the stock path.
inline bool Enabled() { return DodhRakbanClaim::kRecipe == ReservedRecipe; }

struct Offer {
    uint32_t character;
    uint32_t account;
    int16_t slot;
    int32_t shroud;
    // Exact native child slot and serial, in fragment item order.
    std::array<uint8_t, 5> cells{};
    std::array<int32_t, 5> fragments{};
    bool operator==(const Offer&) const = default;
};

std::optional<Offer> Capture(Client&, int16_t slot, const EQ::ItemInstance&);
void Manual(Client&, const Offer&, const DBTradeskillRecipe_Struct&);
}
