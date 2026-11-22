#pragma once

#include <algorithm>

#include "common/housing/legacy_scaling_state.h"
#include "common/repositories/inventory_repository.h"
#include "common/strings.h"
#include "zone/client.h"
#include "zone/corpse.h"
#include "zone/dynamic_zone.h"
#include "zone/npc.h"
#include "zone/zone.h"
#include "zone/zonedb.h"

namespace PorDeathknellHarp {
inline constexpr uint32 Item = 52602;
inline constexpr uint32 ZoneID = 381;
inline constexpr char Expedition[] = "Deathknell";
inline constexpr char ChestTag[] = "por_deathknell:harp_chest";
inline constexpr char OwnerTag[] = "por_deathknell_harp_owner";
inline constexpr char OccurrenceTag[] = "por_deathknell_harp_occurrence";

inline std::string Event(uint32 npc_type)
{
    switch (npc_type) {
    case 381978: return "The Tolling of Dissonance Bells";
    case 381979: return "Vertigo";
    case 381980: return "Unwelcome Guests";
    case 381981: return "The Mad Choir";
    default: return {};
    }
}

inline DynamicZone* CurrentExpedition()
{
    if (!zone || zone->GetZoneID() != ZoneID || zone->GetInstanceVersion() != 20 ||
        !zone->GetInstanceID()) {
        return nullptr;
    }
    auto* dz = zone->GetDynamicZone();
    return dz && dz->IsCurrentZoneDz() && dz->GetType() == DynamicZoneType::Expedition &&
        dz->GetName() == Expedition && !dz->IsExpired() ? dz : nullptr;
}

inline void Capture(Corpse& corpse, NPC& source)
{
    const auto event = Event(source.GetNPCTypeID());
    auto* dz = event.empty() ? nullptr : CurrentExpedition();
    if (dz && source.GetEntityVariable(ChestTag) == dz->GetUUID() + ":" + event) {
        corpse.SetEntityVariable(ChestTag, dz->GetUUID() + ":" + event);
    }
}

inline bool OwnsLoot(Client& client, Corpse& corpse)
{
    const auto event = Event(corpse.GetNPCTypeID());
    auto* dz = event.empty() ? nullptr : CurrentExpedition();
    if (!dz || client.IsSpiritShrouded() || !corpse.IsNPCCorpse() ||
        !dz->HasMember(client.CharacterID()) ||
        corpse.GetEntityVariable(ChestTag) != dz->GetUUID() + ":" + event) {
        return false;
    }
    auto binding = dz->GetLootEvent(corpse.GetID(), DzLootEvent::Type::Entity);
    if (binding.empty()) {
        binding = dz->GetLootEvent(corpse.GetNPCTypeID(), DzLootEvent::Type::NpcType);
    }
    const auto* lockout = client.GetDzLockout(Expedition, event);
    return binding == event && lockout && lockout->IsUUID(dz->GetUUID()) && !lockout->IsExpired() &&
        std::ranges::any_of(dz->GetLockouts(), [&](const DzLockout& earned) {
            return earned.IsEvent(event) && earned.IsUUID(dz->GetUUID()) && !earned.IsExpired();
        });
}

inline bool FlagFromInventory(Client& client, uint64 guid, const std::string& custom_data)
{
    // The actual saved item is the receipt. An in-memory item or a failed save cannot write the flag.
    if (!guid || custom_data.empty()) {
        return false;
    }
    const auto result = database.QueryDatabase(fmt::format(
        "INSERT INTO zone_flags (charID, zoneID) "
        "SELECT character_id, {} FROM inventory WHERE character_id={} AND item_id={} "
        "AND guid={} AND BINARY custom_data=BINARY '{}' LIMIT 1 "
        "ON DUPLICATE KEY UPDATE zoneID=VALUES(zoneID)",
        ZoneID, client.CharacterID(), Item, guid, Strings::Escape(custom_data)));
    if (!result.Success()) {
        return false;
    }
    const auto flag = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM zone_flags WHERE charID={} AND zoneID={}", client.CharacterID(), ZoneID));
    if (!flag.Success() || flag.RowCount() != 1) {
        return false;
    }
    if (!client.HasZoneFlag(ZoneID)) {
        client.LoadZoneFlags();
    }
    return client.HasZoneFlag(ZoneID);
}

struct Transfer {
    Client* client;
    EQ::ItemInstance* item;
    Transfer* previous;
    const bool reserved;
    const bool authorized;
    bool placed = false;
    bool saved = false;
    bool recorded = false;
    inline static thread_local Transfer* current = nullptr;

    Transfer(Client& recipient, EQ::ItemInstance& instance, Corpse& corpse)
        : client(&recipient), item(&instance), previous(current),
          reserved(instance.GetID() == Item && corpse.IsNPCCorpse() &&
              zone && zone->GetZoneID() == ZoneID && zone->GetInstanceVersion() == 20 &&
              !Event(corpse.GetNPCTypeID()).empty()),
          authorized(reserved && !instance.IsStackable() && !instance.IsClassBag() &&
              instance.GetSerialNumber() > 0 && OwnsLoot(recipient, corpse))
    {
        if (authorized) {
            instance.SetCustomData(OwnerTag, std::to_string(recipient.CharacterID()));
            instance.SetCustomData(OccurrenceTag, corpse.GetEntityVariable(ChestTag));
            current = this;
        }
    }

    ~Transfer() { if (authorized) current = previous; }
    Transfer(const Transfer&) = delete;
    Transfer& operator=(const Transfer&) = delete;
};

inline void InventorySave(Client& client, const EQ::ItemInstance& item, bool placed, bool saved)
{
    auto* transfer = Transfer::current;
    if (transfer && transfer->client == &client && transfer->item == &item) {
        transfer->placed = placed;
        transfer->saved = placed && saved;
        if (transfer->saved) {
            transfer->recorded = FlagFromInventory(client, item.GetSerialNumber(), item.GetCustomDataString());
        }
    }
}

inline void Reconcile(Client& client)
{
    if (client.IsSpiritShrouded() || client.HasZoneFlag(ZoneID) ||
        client.GetInv().HasItem(Item, 1) == EQ::invslot::SLOT_INVALID) {
        return;
    }
    const auto items = InventoryRepository::GetWhere(database, fmt::format(
        "character_id={} AND item_id={}", client.CharacterID(), Item));
    const auto owner = std::to_string(client.CharacterID());
    for (const auto& row : items) {
        const auto fields = Housing::LegacyScaling::Fields(row.custom_data);
        if (!fields) {
            continue;
        }
        const auto owner_field = fields->find(OwnerTag);
        const auto occurrence = fields->find(OccurrenceTag);
        if (owner_field == fields->end() || owner_field->second != owner ||
            occurrence == fields->end() || occurrence->second.size() <= 37 ||
            occurrence->second[36] != ':') {
            continue;
        }
        const auto event = occurrence->second.substr(37);
        if ((event == Event(381978) || event == Event(381979) || event == Event(381980) ||
            event == Event(381981)) && FlagFromInventory(client, row.guid, row.custom_data)) {
            return;
        }
    }
}
}
