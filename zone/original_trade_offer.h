#pragma once
#include "../common/shared_core/original_trade_recipe.h"
// Included where Client and native inventory constants are already defined.
// Does not accept a Lua item map, caller-generated GUID, or cloned item vector.
namespace OriginalTradeOffer {
inline EQEmu::SharedCore::OriginalTradeRecipe::Offers Capture(Client& client) {
 namespace Recipe=EQEmu::SharedCore::OriginalTradeRecipe;
 static_assert(EQ::invslot::TRADE_NPC_END-EQ::invslot::TRADE_BEGIN+1==Recipe::OfferCount,"NPC offer boundary changed");
 Recipe::Offers offers{};
 for(size_t i=0;i<Recipe::OfferCount;++i) {
  const auto slot=int16_t(EQ::invslot::TRADE_BEGIN+i);
  if(auto* item=client.GetInv().GetItem(slot))
   offers[i]={true,slot,item->GetSerialNumber(),item->GetID(),item->GetCharges(),item->IsStackable()};
 }
 return offers;
}
}
