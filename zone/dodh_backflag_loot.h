#pragma once

#include "zone/client.h"
#include "zone/corpse.h"
#include "zone/dynamic_zone.h"
#include "zone/zone.h"

namespace DodhBackflag {
inline constexpr uint32_t Item = 88036;
inline constexpr char Expedition[] = "Dreadspire Keep: Demi-Plane of Blood";
inline constexpr char CorpseMarker[] = "dodh_demi:backflag";
inline constexpr char ItemOwner[] = "dodh_backflag_recipient";
inline constexpr char Receipt[] = "dodh_blood:backflag";

inline bool OwnsReward(Client& client, Corpse& corpse)
{
    if (!zone || zone->GetZoneID() != 351 || zone->GetInstanceVersion() != 1 ||
        !zone->GetInstanceID() || client.IsSpiritShrouded() || !corpse.IsNPCCorpse()) {
        return false;
    }
    auto* expedition = zone->GetDynamicZone();
    return expedition && expedition->GetType() == DynamicZoneType::Expedition &&
        expedition->GetName() == Expedition &&
        expedition->GetInstanceID() == zone->GetInstanceID() && !expedition->IsExpired() &&
        expedition->HasMember(client.CharacterID()) &&
        corpse.GetEntityVariable(CorpseMarker) == expedition->GetUUID();
}

inline void Record(Client& client)
{
    if (client.GetBucket(Receipt) != "1") {
        client.SetBucket(Receipt, "1");
    }
    if (client.GetAA(1378) < 5) {
        client.GrantAlternateAdvancementAbility(488, 5, true);
    }
}

struct Transfer {
    Client* client;
    EQ::ItemInstance* item;
    Transfer* previous;
    const bool owned;
    bool placed = false;
    bool saved = false;

    inline static thread_local Transfer* current = nullptr;

    Transfer(Client& recipient, EQ::ItemInstance& instance, Corpse& corpse)
        : client(&recipient), item(&instance), previous(current),
          owned(instance.GetID() == Item && !instance.IsStackable() && OwnsReward(recipient, corpse))
    {
        if (owned) {
            instance.SetCustomData(ItemOwner, std::to_string(recipient.CharacterID()));
            current = this;
        }
    }

    ~Transfer() { if (owned) current = previous; }
    Transfer(const Transfer&) = delete;
    Transfer& operator=(const Transfer&) = delete;
};

inline void InventorySave(Client& client, const EQ::ItemInstance& item, bool placed, bool saved)
{
    auto* transfer = Transfer::current;
    if (!transfer || transfer->client != &client || transfer->item != &item) {
        return;
    }
    transfer->placed = placed;
    transfer->saved = placed && saved;
}

inline void Reconcile(Client& client)
{
    if (client.IsSpiritShrouded() || (client.GetAA(1378) >= 5 && client.GetBucket(Receipt) == "1")) {
        return;
    }
    auto& inventory = client.GetInv();
    const auto owned = [&client](EQ::ItemInstance* item) {
        return item && item->GetID() == Item &&
            item->GetCustomData(ItemOwner) == std::to_string(client.CharacterID());
    };
    const auto slot = inventory.HasItem(Item, 1);
    if (slot == EQ::invslot::slotCursor) {
        for (auto it = inventory.cursor_cbegin(); it != inventory.cursor_cend(); ++it) {
            if (owned(*it)) {
                Record(client);
                return;
            }
        }
    } else if (slot >= 0 && owned(inventory.GetItem(slot))) {
        Record(client);
    }
}
}
