#pragma once

#include "original_trade_recipe.h"

#include <array>
#include <string>
#include <string_view>

namespace EQEmu::SharedCore::PorTunare {

inline constexpr uint32_t Elddar = 378;
inline constexpr uint32_t Shrine = 379;
inline constexpr uint32_t Raid = 920349;
inline constexpr uint32_t Guardian = 379942;
inline constexpr uint32_t Chalice = 85085;
inline constexpr uint32_t Augmentation = 85670;
inline constexpr char GuardianTag[] = "por_tunare:guardian";
inline constexpr char OrgafTag[] = "por_tunare:orgaf";
inline constexpr char RewardTag[] = "por_tunare:reward_run";

inline const char* CorpseMarker(uint32_t zone, uint32_t version, uint32_t instance, uint32_t npc)
{
    if (zone == Shrine && version == 22 && instance && npc == Guardian) return GuardianTag;
    if (zone != Elddar || version || instance) return nullptr;
    if (npc == 378168) return OrgafTag;
    if (npc >= 379947 && npc <= 379949) return RewardTag;
    return nullptr;
}

inline bool HexDigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

inline bool CorpseMarkerValue(uint32_t npc, std::string_view value)
{
    if (npc == Guardian) {
        if (value.size() != 36) return false;
        for (std::size_t i = 0; i < value.size(); ++i) {
            const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
            if (dash ? value[i] != '-' : !HexDigit(value[i])) return false;
        }
        return true;
    }
    if (value.empty() || value.size() > 19 || value.front() == '0') return false;
    for (const auto c : value) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}
inline constexpr char OwnerTag[] = "por_tunare_chalice_owner";
inline constexpr char SourceTag[] = "por_tunare_chalice_source";

enum class Give : uint8_t { Investigating = 1, Questioning, Orgaf, Final };

struct Route {
    Give give;
    uint32_t npc;
    uint32_t spawn;
    uint32_t task;
    uint32_t predecessor;
    uint32_t activity;
    std::array<uint32_t, 4> inputs;
};

inline constexpr std::array Routes{
    Route{Give::Investigating, 378059, 104960, 920346, 4, 5, {85078}},
    Route{Give::Questioning, 378059, 104960, 920347, 2, 3, {85080}},
    Route{Give::Orgaf, 378168, 105177, 920348, 8, 9, {85081, 85082, 85083, 85084}},
    Route{Give::Final, 378059, 104960, 0, 0, 0, {Chalice}},
};

inline std::string Context(const Route& route)
{
    return std::string("TUN1") + static_cast<char>(route.give);
}

inline const Route* Decode(std::string_view context)
{
    if (context.size() != 5 || context.substr(0, 4) != "TUN1") return nullptr;
    for (const auto& route : Routes) {
        if (static_cast<char>(route.give) == context[4]) return &route;
    }
    return nullptr;
}

inline bool Matches(const Route& route, uint32_t zone, uint32_t instance, uint32_t npc, uint32_t spawn)
{
    return zone == Elddar && !instance && npc == route.npc && spawn == route.spawn;
}

inline bool Input(const Route& route, uint32_t item)
{
    for (const auto input : route.inputs) {
        if (input && item == input) return true;
    }
    return false;
}

inline bool Plan(const Route& route, const OriginalTradeRecipe::Offers& offers,
    OriginalTradeRecipe::DebitPlan& debit)
{
    OriginalTradeRecipe::DebitPlan plan;
    for (const auto input : route.inputs) {
        if (!input) break;
        OriginalTradeRecipe::DebitPlan unit;
        if (!OriginalTradeRecipe::FirstUnit(offers, input, unit)) return false;
        for (std::size_t i = 0; i < unit.units.size(); ++i) plan.units[i] += unit.units[i];
    }
    if (!OriginalTradeRecipe::Valid(offers, plan)) return false;
    debit = plan;
    return true;
}

inline bool SourceKey(std::string_view key)
{
    if (key.size() != 64) return false;
    for (const auto c : key) {
        if (!HexDigit(c)) return false;
    }
    return true;
}
}
