#pragma once
#include <cstdint>
namespace EQEmu::SharedCore {
// Trusted authenticated server session identity; never populate from wire fields.
struct Actor { uint32_t character_id=0; uint32_t account_id=0; };
}
namespace Housing { using AuthenticatedPurchaser = EQEmu::SharedCore::Actor; }
