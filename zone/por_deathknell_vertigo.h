#pragma once

#include <optional>

class Client;
class Mob;
class Trap;

namespace PorDeathknellVertigo {
struct Counts {
    int phase = 0;
    unsigned phase_count = 0;
    unsigned cumulative_count = 0;
};

bool IsReserved(const Trap& trap);
bool CanInteract(const Trap& trap, Mob* actor);
void OnDisarm(Trap& trap, Client& client);
bool SetPhase(int phase);
std::optional<Counts> GetCounts();
void Clear();
void ResetForQuestReload();
}
