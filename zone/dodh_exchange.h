#pragma once

class Client;
class NPC;

namespace DodhOriginalExchange {
bool ClaimsOffer(Client&, NPC&);
bool ClaimsMonocleOffer(Client&, NPC&);
bool ClaimsPublicRoute(Client&, NPC&);
bool OriginalAccept(Client&, NPC&);
bool ClaimsGuardOffer(Client&, NPC&);
bool ClaimsOpenerOffer(Client&, NPC&);
bool ClaimsLongshadowOffer(Client&, NPC&);
bool ClaimsPorChimeOffer(Client&, NPC&);
bool ClaimsTunareOffer(Client&, NPC&);
}
