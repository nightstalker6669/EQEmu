#pragma once

#include "common/shared_core/vergalid_mission.h"
#include <string>

class Client;
class Mob;
class NPC;

namespace VergalidMission {
std::string UUID(Client&, uint32_t task);
std::string EventStatus(Client&, const std::string& event);
bool AwardEvent(Client&, const std::string& event);
bool EventLoot(Client&, const std::string& event);
bool ReceiveDoomingExile(Client&, Mob* caster);

bool ReservedHazard(const NPC&);
NPC* HazardTarget(Client&);
}
