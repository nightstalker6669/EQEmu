#pragma once

#include "dodh_longshadow_custody.h"

namespace EQEmu::SharedCore::DodhLongshadowCustody {
inline const char* SourceKind(Source source)
{
    switch (source) {
    case Source::DustspiritNote: return "dustspirit_note";
    case Source::ShadowspinCorpse: return "shadowspin_corpse";
    case Source::PoisonmasterCorpse: return "poisonmaster_corpse";
    case Source::RyiannaOrb: return "ryianna_orb";
    case Source::TraitorChest: return "traitor_chest";
    case Source::MinesMemory: return "bellfast_mines_memory";
    case Source::ClicktokGem: return "bellfast_clicktok_gem";
    case Source::TherimelGem: return "bellfast_therimel_gem";
    case Source::ZlixtilGem: return "bellfast_zlixtil_gem";
    case Source::BilitanGem: return "bellfast_bilitan_gem";
    case Source::AlphaLarvaMeat: return "bellfast_alpha_larva_meat";
    }
    return nullptr;
}

enum class Origin { Acceptance, Hail, Corpse, Ground };

struct SourceRoute {
    uint32_t task;
    uint32_t activity;
    uint32_t item;
    Source source;
    Origin origin;
    uint32_t zone;
    uint32_t version;
    uint32_t npc;
    uint32_t spawn;
    bool publishes_activity;
};

inline constexpr std::array<SourceRoute, 11> Sources{{
    {920251, 0, 54606, Source::DustspiritNote, Origin::Acceptance, 358, 0, 358123, 102235, false},
    {920251, 3, 54607, Source::ShadowspinCorpse, Origin::Corpse, 354, 1, 354920, 0, true},
    {920252, 2, 54608, Source::PoisonmasterCorpse, Origin::Corpse, 354, 5, 354311, 262895, true},
    {920253, 0, 54609, Source::RyiannaOrb, Origin::Hail, 358, 0, 358128, 102247, true},
    {920255, 1, 54608, Source::TraitorChest, Origin::Corpse, 363, 3, 363053, 0, true},
    {920270, 1, 87005, Source::MinesMemory, Origin::Ground, 367, 1, 0, 0, true},
    {920270, 2, 87004, Source::ClicktokGem, Origin::Corpse, 367, 1, 367051, 259691, true},
    {920271, 4, 87008, Source::TherimelGem, Origin::Corpse, 360, 1, 360056, 259600, true},
    {920272, 4, 87009, Source::ZlixtilGem, Origin::Corpse, 355, 1, 355940, 0, true},
    {920273, 5, 87010, Source::BilitanGem, Origin::Corpse, 349, 2, 349940, 0, true},
    {920270, 2, 87014, Source::AlphaLarvaMeat, Origin::Corpse, 367, 1, 367059, 259704, false},
}};

inline const SourceRoute* FindSourceActivity(uint32_t task, uint32_t activity)
{
    for (const auto& route : Sources) {
        if (route.publishes_activity && route.task == task && route.activity == activity) return &route;
    }
    return nullptr;
}
}
