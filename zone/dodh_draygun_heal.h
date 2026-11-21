#pragma once

#include <cstdint>

class Mob;

namespace DodhDraygunHeal {
bool Allows(Mob& target, Mob* caster, std::int32_t spell_id);
}
