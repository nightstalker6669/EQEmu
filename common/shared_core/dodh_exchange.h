#pragma once

#include "original_trade_recipe.h"
#include <algorithm>
#include <optional>
#include <string>
#include <string_view>

namespace EQEmu::SharedCore::DodhExchange {
inline constexpr unsigned JournalKind = 4;
inline constexpr char JournalContext[] = "DODH\x01\x01";
inline constexpr uint32_t Zone = 365;
inline constexpr uint32_t Giver = 365284;
inline constexpr uint32_t Spawn = 103431;
inline constexpr uint32_t Output = 88005;
inline constexpr uint32_t OpenerNpc = 365275;
inline constexpr uint32_t OpenerSpawn = 103414;
inline constexpr uint32_t Notebook = 84040;
inline constexpr char OpenerContext[] = "DODH\x01\x03";
inline constexpr uint32_t Guard = 86018;
inline constexpr uint32_t DesertingTask = 920224;
inline constexpr uint32_t DesertingZone = 350;
inline constexpr uint32_t DesertingVersion = 1;
inline constexpr std::string_view GuardPrefix{"DODH\x01\x02", 6};

inline bool GuardActor(uint32_t npc, uint32_t spawn)
{
    switch (spawn) {
    case 263220: return npc == 350035;
    case 263223: return npc == 350037;
    case 263226: return npc == 350038;
    case 263221: case 263222: case 263224: case 263225: case 263227:
    case 263228: case 263229: case 263231: case 263232: return npc == 350036;
    default: return false;
    }
}

struct GuardIdentity {
    uint64_t run;
    uint32_t dz;
    uint32_t entity;
    uint32_t generation;
};

inline std::string EncodeGuard(const GuardIdentity& id)
{
    std::string out(GuardPrefix);
    const auto append = [&](uint64_t value, unsigned width) {
        for (unsigned i = 0; i < width; ++i) out += char(uint8_t(value >> (8 * i)));
    };
    append(id.run, 8);
    append(id.dz, 4);
    append(id.entity, 4);
    append(id.generation, 4);
    return out;
}

inline std::optional<GuardIdentity> DecodeGuard(std::string_view context)
{
    if (context.size() != GuardPrefix.size() + 20 || context.substr(0, GuardPrefix.size()) != GuardPrefix)
        return std::nullopt;
    std::size_t offset = GuardPrefix.size();
    const auto read = [&](unsigned width) {
        uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i) value |= uint64_t(uint8_t(context[offset++])) << (8 * i);
        return value;
    };
    GuardIdentity id{read(8), uint32_t(read(4)), uint32_t(read(4)), uint32_t(read(4))};
    if (!id.run || !id.dz || !id.entity || !id.generation) return std::nullopt;
    return id;
}
inline constexpr std::array<uint32_t, 4> Parts{88001, 88010, 88011, 88012};

inline bool IsKind(unsigned kind)
{
    return kind == JournalKind;
}

inline bool IsPart(uint32_t item)
{
    return std::find(Parts.begin(), Parts.end(), item) != Parts.end();
}

inline bool Plan(const OriginalTradeRecipe::Offers& offers, OriginalTradeRecipe::DebitPlan& debit)
{
    debit = {};
    OriginalTradeRecipe::DebitPlan candidate;
    for (const auto item : Parts) {
        const auto found = std::find_if(offers.begin(), offers.end(), [item](const auto& offer) {
            return offer.present && offer.item_id == item;
        });
        if (found == offers.end()) {
            return false;
        }
        candidate.units[found - offers.begin()] = 1;
    }
    if (!OriginalTradeRecipe::Valid(offers, candidate)) {
        return false;
    }
    debit = candidate;
    return true;
}
}
