#pragma once
#include "original_trade_recipe_hooks.h"
// Spells implements these native semantic bindings. ClaimsOffer must use exact
// bound giver/item IDs and the feature gate, never names/Lua aggregate counts.
// Missing policy/IDs: no eligible admission and no new Food task activation.
namespace SodFoodDelivery {
bool ClaimsOffer(Client&,NPC&);
OriginalTradeWriter::SemanticRecipe& RecipeHooks();
}
namespace OriginalTradeFood {
// True owns original accept INCLUDING refused preflight; never generic fallback.
bool OriginalAccept(Client&,NPC&);
bool RejectUnissued(Client&,NPC&);
}
