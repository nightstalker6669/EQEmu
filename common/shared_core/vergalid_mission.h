#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace VergalidMission {
inline constexpr uint32_t Zone = 404;
inline constexpr uint32_t Kellak = 920360;
inline constexpr uint32_t End = 920361;
inline constexpr uint32_t EndTemplate = 920365;
inline constexpr char Name[] = "Vergalid Mines: Vergalid's End";
inline constexpr uint32_t Dragon = 404959;
inline constexpr uint32_t DoomingExile = 11424;
inline constexpr char DragonTag[] = "tss_vergalid:dragon";
inline constexpr char Hazard[] = "tss_vergalid:hazard";
inline constexpr char DisarmedBy[] = "tss_vergalid:disarmed_by";

struct Event {
    std::string_view name;
    uint32_t seconds;
};
inline constexpr std::array Events{
    Event{"Nurtha Frazzlesprocket", 302400},
    Event{"Goru Uldrock", 216000},
    Event{"Vergalid", 302400},
};
inline const Event* FindEvent(std::string_view name)
{
    for (const auto& event : Events) if (event.name == name) return &event;
    return nullptr;
}
inline constexpr uint32_t Version(uint32_t task)
{
    return task == Kellak ? 20 : task == End ? 21 : 0;
}
inline constexpr bool HazardType(uint32_t npc)
{
    return npc == 404946 || npc == 404947;
}
}
