#pragma once

#include "common/shared_core/por_tunare_exchange.h"

class Client;
class Corpse;
class NPC;

namespace EQ { class ItemInstance; }

namespace TunareCustody {
enum class LootResult { Unrelated, Refused, Issued, AlreadyIssued, InventoryReload };

void Capture(Corpse&, NPC&);
LootResult Loot(Client&, Corpse&, const EQ::ItemInstance&, uint16_t loot_slot);
bool Reconcile(Client&);
}
