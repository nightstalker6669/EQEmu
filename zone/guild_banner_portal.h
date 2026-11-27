#ifndef EQEMU_GUILD_BANNER_PORTAL_H
#define EQEMU_GUILD_BANNER_PORTAL_H

#include "guild_banner_store.h"

class EQApplicationPacket;
class Mob;

namespace GuildBanner {

// Preflight is read-only. Confirmation checks the same revision again.
std::optional<Banner> PortalDestination(Client& client, uint32_t guild_id);
bool TravelFromPortal(Client& client, uint32_t guild_id, uint64_t revision);
bool CanAlignBasicPortal(Client& client, Mob* merchant);
bool OfferBasicPortal(Client& client, Mob* porter);
void ProcessBasicPortalEntry(Client& client);
bool HandleBasicPopup(Client& client, const EQApplicationPacket* packet);

} // namespace GuildBanner

#endif
