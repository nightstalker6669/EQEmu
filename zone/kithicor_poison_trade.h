#pragma once
#include <cstdint>
class Client;
class NPC;
namespace EQ {class ItemInstance;}
namespace KithicorPoisonTrade {
class Writer;
// Original accept interception. True owns this attempt, including rejection.
bool OriginalAccept(Client&,NPC&);
// Fail closed for a direct/unissued generic FinishTrade against these chests.
bool RejectUnissued(Client&,NPC&);
// Spells task handler ONLY: consume once during actual original hand-in callback.
// False must never INSERT status=1 or SetTaskDeliveredCount. Proof is bound to
// the actual pointer plus current client/run/phase/instance/chest identity.
bool ConsumeAcknowledgment(Client&,NPC&,const EQ::ItemInstance&);
}
