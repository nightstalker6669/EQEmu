#pragma once

#include <cstdint>
#include <glm/vec4.hpp>

class Client;
class Doors;
class NPC;
class Corpse;
class Mob;

namespace DodhLongshadowTravel {
void RecordArrival(Client&);
bool UseDoor(Client&, Doors&);
bool PrepareOrbDeath(NPC&, Mob*);
bool IsOrbCorpse(Corpse&);
bool IsTraitorCorpse(Corpse&);
bool IsFinaleActor(NPC&);
bool AllowCavernsDeath(NPC&, Mob*);
bool PrepareBellfastLoot(NPC&, Mob*);
bool IsTraitorChest(NPC&);
bool AllowTraitorOpen(Client&, NPC&);
bool PrepareFinaleDeath(NPC&, Mob*);
int64_t CapFinaleDamage(NPC&, Mob*, int64_t);
}

namespace DodhLongshadowScene {
void Replay();
int64_t CapDamage(NPC&, Mob*, int64_t);
bool RecordDeath(NPC&, Mob*);
void StampCorpse(NPC&, Corpse&);
bool AllowLoot(Client&, Corpse&);
bool AllowPleaOpen(Client&, NPC&);
bool ReplayOwned(Mob&);
}
namespace EQ { class ItemInstance; }

namespace DodhLongshadowSource {
enum class LootResult { Unrelated, Refused, Issued, AlreadyIssued, InventoryReload };

void Accepted(Client&, NPC&, uint32_t task, uint32_t accepted_time);
void RetryPendingNote(Client&);
bool OnTargetedSay(Client&, NPC&, const char*);
void MarkNativeCorpse(NPC&, Corpse&);
LootResult NativeLoot(Client&, Corpse&, const EQ::ItemInstance&);
LootResult NativeBellfastGroundPickup(Client&, const EQ::ItemInstance&);
}


namespace DodhBellfastNeran {
bool Open(Client&, NPC&);
bool Hail(Client&, NPC&);
bool CanEnter(Client&);
void ForceLanded(NPC&, Client&);
void ObserveMovement(Client&, bool);
void Progress(Client&);
}

namespace DodhBellfastKorlach {
void RecordTrashDeath(NPC&, Mob*);
bool PrepareDeath(NPC&, Mob*);
void Replay(Client&);
}

namespace DodhBellfastHive {
bool IsChest(NPC&);
bool AllowOpen(Client&, NPC&);
bool PrepareDeath(NPC&, Mob*);
void Replay(Client&);
}

namespace DodhHardNotebookChest {
bool IsActor(NPC&);
bool IsCorpse(Corpse&);
bool AllowOpen(Client&, NPC&);
bool PrepareDeath(NPC&, Mob*);
void Replay(Client&);
}
