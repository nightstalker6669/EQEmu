#ifndef EQEMU_ZONE_BAZAAR_OFFLINE_STALL_H
#define EQEMU_ZONE_BAZAAR_OFFLINE_STALL_H

#include <cstdint>

class Client;
class Zone;

namespace BazaarStalls {
void Process(Zone &zone);
void Clear(Zone &zone);
void Send(Client &client);
uint32_t CounterSeller(Client &buyer, uint32_t entity);
}
#endif
