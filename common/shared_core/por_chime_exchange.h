#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace EQEmu::SharedCore::PorChimeExchange {
struct Route {
    uint32_t zone, npc, spawn, input, output, prerequisite;
    char code;
};
inline constexpr std::array Routes{
    Route{369, 369011, 103459, 84159, 84160, 920336, '1'},
    Route{372, 372000, 103726, 84160, 84161, 920306, '2'},
    Route{369, 369011, 103459, 84161, 84168, 920306, '3'},
    Route{392, 392088, 105896, 84168, 84162, 920309, '4'},
};
inline constexpr std::string_view Prefix{"POR\x01", 4};

inline std::string Encode(const Route& route)
{
    std::string context(Prefix);
    context.push_back(route.code);
    return context;
}

inline const Route* Decode(std::string_view context)
{
    if (context.size() != Prefix.size() + 1 || context.substr(0, Prefix.size()) != Prefix) return nullptr;
    for (const auto& route : Routes) {
        if (route.code == context.back()) return &route;
    }
    return nullptr;
}

inline bool Matches(const Route& route, uint64_t zone, uint64_t instance, uint64_t npc, uint64_t spawn)
{
    return instance == 0 && route.zone == zone && route.npc == npc && route.spawn == spawn;
}
}
