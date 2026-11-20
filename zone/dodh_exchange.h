#pragma once

class Client;
class NPC;

namespace DodhOriginalExchange {
bool ClaimsOffer(Client&, NPC&);
bool OriginalAccept(Client&, NPC&);
bool ClaimsGuardOffer(Client&, NPC&);
bool ClaimsOpenerOffer(Client&, NPC&);
}
