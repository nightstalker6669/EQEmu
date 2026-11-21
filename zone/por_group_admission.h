#pragma once

namespace PorGroupAdmission {
struct Request {
    uint32_t task, zone, npc, spawn, minimum, prerequisite, item;
};

inline constexpr std::array Requests{
    Request{920300, 369, 369083, 103665, 70, 0, 0},
    Request{920301, 369, 369083, 103665, 70, 920300, 0},
    Request{920302, 369, 369083, 103665, 70, 920301, 0},
    Request{920305, 202, 202125,  58619, 70, 920304, 0},
    Request{920306, 372, 372000, 103726, 70, 920305, 88099},
    Request{920307, 392, 392045, 105817, 70, 0, 0},
    Request{920308, 392, 392045, 105817, 70, 920307, 0},
    Request{920309, 392, 392045, 105817, 70, 920308, 0},
};

inline bool AllowsRequest(Client& client, uint32_t task, NPC* giver, const Zone* current)
{
    const auto request = std::find_if(Requests.begin(), Requests.end(), [task](const Request& row) {
        return row.task == task;
    });
    if (request == Requests.end()) {
        return true;
    }
    return current && current->GetZoneID() == request->zone && current->GetInstanceVersion() == 0 &&
        current->GetInstanceID() == 0 && giver && giver->GetNPCTypeID() == request->npc &&
        giver->GetSpawnPointID() == request->spawn && !giver->GetDepop() && giver->GetHP() > 0 &&
        !client.IsSpiritShrouded() && client.GetLevel() >= request->minimum &&
        (!request->prerequisite || client.IsTaskCompleted(request->prerequisite)) &&
        (!request->item || client.GetInv().HasItem(request->item, 1, invWhereWorn | invWherePersonal | invWhereCursor) != INVALID_INDEX);
}
}
