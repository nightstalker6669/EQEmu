#pragma once

#include "original_trade_recipe.h"
#include <array>
#include <optional>

namespace EQEmu::SharedCore::DodhLongshadowCustody {
namespace Trade = OriginalTradeRecipe;

enum class Source { DustspiritNote, ShadowspinCorpse, PoisonmasterCorpse, RyiannaOrb, TraitorChest, MinesMemory, ClicktokGem, TherimelGem, ZlixtilGem, BilitanGem, AlphaLarvaMeat };
enum class Effect { SpawnShadowspin, FreeElder, ReportPoisonmaster, OpenBloodeyeChest, ReportTraitor };

struct Route {
    uint32_t task;
    uint32_t activity;
    uint32_t item;
    uint32_t zone;
    uint32_t version;
    uint32_t npc;
    uint32_t spawn;
    Source source;
    Effect effect;
};

// Exact Longshadow Normal task IDs also used by the Rakban archive.
inline constexpr std::array<Route, 5> NormalRoutes{{
        {920251, 1, 54606, 354, 1, 354150, 262068, Source::DustspiritNote, Effect::SpawnShadowspin},
        {920251, 4, 54607, 354, 1, 354150, 262068, Source::ShadowspinCorpse, Effect::FreeElder},
        {920252, 3, 54608, 358, 0, 358123, 102235, Source::PoisonmasterCorpse, Effect::ReportPoisonmaster},
        {920253, 3, 54609, 363, 1, 363029, 260753, Source::RyiannaOrb, Effect::OpenBloodeyeChest},
        {920255, 4, 54608, 358, 0, 358123, 102235, Source::TraitorChest, Effect::ReportTraitor},
}};

struct Context {
    uint64_t occurrence;
    uint32_t accepted_time;
    uint32_t character;
    uint32_t task;
    uint32_t activity;
    uint32_t mission_instance;
    uint32_t zone;
    uint32_t version;
    uint32_t instance;
    uint32_t npc;
    uint32_t spawn;
    bool activity_active;
};

struct SourceReceipt {
    uint64_t occurrence;
    uint32_t accepted_time;
    uint32_t character;
    uint32_t task;
    uint32_t mission_instance;
    uint32_t item;
    int32_t guid;
    Source source;
};

inline std::optional<Trade::DebitPlan> Plan(
    const Route& route, const Context& context, const SourceReceipt& source, const Trade::Offers& offers)
{
    if (!route.task || !context.character || !context.occurrence || !context.accepted_time || !context.mission_instance ||
        !context.activity_active || context.task != route.task || context.activity != route.activity ||
        context.zone != route.zone || context.version != route.version ||
        context.npc != route.npc || context.spawn != route.spawn ||
        context.instance != (route.zone == 358 ? 0 : context.mission_instance) ||
        source.occurrence != context.occurrence || source.accepted_time != context.accepted_time ||
        source.character != context.character ||
        source.task != route.task || source.mission_instance != context.mission_instance ||
        source.item != route.item || source.source != route.source || source.guid <= 0) {
        return std::nullopt;
    }

    for (std::size_t i = 0; i < offers.size(); ++i) {
        const auto& offer = offers[i];
        if (!offer.present || offer.item_id != route.item || offer.guid != source.guid) {
            continue;
        }
        Trade::DebitPlan debit;
        debit.units[i] = 1;
        if (Trade::Valid(offers, debit)) {
            return debit;
        }
    }
    return std::nullopt;
}
inline constexpr bool TaskOnly(Effect effect)
{
    return effect == Effect::FreeElder || effect == Effect::ReportPoisonmaster ||
        effect == Effect::ReportTraitor;
}

inline constexpr uint32_t SceneActor(Effect effect)
{
    switch (effect) {
    case Effect::SpawnShadowspin: return 354920;
    case Effect::OpenBloodeyeChest: return 363035;
    default: return 0;
    }
}

inline constexpr bool DeathActivity(uint32_t task, uint32_t activity)
{
    return task == 920251 && activity == 2;
}

inline constexpr bool SceneLoot(uint32_t actor, uint32_t item)
{
    return (actor == 354920 && item == 54607) ||
        (actor == 363035 && item >= 86568 && item <= 86572);
}

inline const Route* FindRoute(uint32_t task, uint32_t activity)
{
    for (const auto& route : NormalRoutes) {
        if (route.task == task && route.activity == activity) return &route;
    }
    return nullptr;
}
}
