#include "bazaar_offline_stall.h"

#include "../common/bazaar_offline_stall.h"
#include "../common/housing/admission_guard.h"
#include "../common/repositories/trader_repository.h"
#include "client.h"
#include "entity.h"
#include "npc.h"
#include "zone.h"
#include "worldserver.h"

#include <chrono>
#include <map>
#include <memory>

namespace BazaarStalls {
namespace {
class OfflineStallActor : public NPC {
public:
	using NPC::NPC;

	void FillSpawnStruct(NewSpawn_Struct *spawn, Mob *viewer) override
	{
		NPC::FillSpawnStruct(spawn, viewer);
		// A trader is a player-shaped shop on the RoF2 wire. Internally the
		// actor stays an NPC with no player inventory, login, or session.
		if (!viewer || (viewer->IsClient() &&
			viewer->CastToClient()->ClientVersion() == EQ::versions::ClientVersion::RoF2)) {
			spawn->spawn.NPC = 0;
			spawn->spawn.is_npc = 0;
			spawn->spawn.trader = true;
			spawn->spawn.guildID = GUILD_NONE;
		}
	}
};

struct Visual {
	OfflineBazaarStall::Stall stall;
	NPC *actor;
};

std::map<uint16_t, Visual> stalls;
std::chrono::steady_clock::time_point next_update;

NPC *Find(uint16_t entity, const Visual &visual)
{
	auto *npc = entity_list.GetNPCByID(entity);
	return npc && npc == visual.actor && !npc->GetDepop() &&
		npc->GetEntityVariable("offline_bazaar_stall") == visual.stall.owner ? npc : nullptr;
}

void SendVisual(Client &client, const OfflineBazaarStall::Stall &stall, uint16_t entity, bool active)
{
	if (client.ClientVersion() != EQ::versions::ClientVersion::RoF2) {
		return;
	}
	EQApplicationPacket packet(OP_BecomeTrader, sizeof(BecomeTrader_Struct));
	auto *data = reinterpret_cast<BecomeTrader_Struct *>(packet.pBuffer);
	data->entity_id = entity;
	data->trader_id = stall.seller;
	data->zone_id = Zones::BAZAAR;
	data->zone_instance_id = stall.instance;
	strn0cpy(data->trader_name, stall.name.c_str(), sizeof(data->trader_name));
	data->action = active ? TraderOn : TraderOff;
	client.QueuePacket(&packet);
}

void Remove(uint16_t entity, const Visual &visual)
{
	if (auto *npc = Find(entity, visual)) {
		for (const auto &[id, client] : entity_list.GetClientList()) {
			SendVisual(*client, visual.stall, entity, false);
		}
		npc->Depop(false);
	}
}

void Restore(const OfflineBazaarStall::Stall &stall)
{
	if (Housing::CheckInventoryAdmission(database, stall.seller, true) != Housing::InventoryAdmission::Allowed ||
		entity_list.GetClientByCharID(stall.seller)) {
		return;
	}
	auto type = std::make_unique<NPCType>();
	strn0cpy(type->name, stall.name.c_str(), sizeof(type->name));
	type->race = 1;
	type->gender = 0;
	type->class_ = 1;
	type->level = 1;
	type->size = 6;
	type->bodytype = 1;
	type->current_hp = type->max_hp = 1;
	type->show_name = true;
	type->skip_global_loot = true;
	type->skip_auto_scale = true;

	const auto &p = stall.position;
	auto actor = std::make_unique<OfflineStallActor>(type.get(), nullptr, glm::vec4(p.x, p.y, p.z, p.heading), GravityBehavior::Flying);
	actor->GiveNPCTypeData(type.release());
	actor->AI_Stop();
	actor->Mob::AI_Stop();
	actor->SetInvul(true);
	actor->SetEntityVariable("offline_bazaar_stall", stall.owner);

	// Reserve a real entity ID before publishing the shop. A failed durable
	// claim never broadcasts a trader or exposes usable rows for the actor.
	entity_list.AddNPC(actor.get(), false, true);
	const auto entity = actor->GetID();
	auto *npc = actor.release();
	if (!OfflineBazaarStall::Recover(database, stall, entity)) {
		npc->Depop(false);
		return;
	}
	stalls.emplace(entity, Visual{stall, npc});

	for (const auto &[id, client] : entity_list.GetClientList()) {
		EQApplicationPacket spawn;
		npc->CreateSpawnPacket(&spawn, client);
		client->QueuePacket(&spawn, true, Mob::CLIENT_CONNECTED);
		SendVisual(*client, stall, entity, true);
	}
}
}

void Process(Zone &zone)
{
	if (zone.GetZoneID() != Zones::BAZAAR || !worldserver.Connected() || !RuleB(Bazaar, EnableOfflineTraders)) {
		return;
	}
	const auto now = std::chrono::steady_clock::now();
	if (now < next_update) {
		return;
	}
	next_update = now + std::chrono::seconds(3);

	for (auto it = stalls.begin(); it != stalls.end();) {
		const auto &visual = it->second;
		auto listings = database.QueryDatabase(fmt::format(
			"SELECT 1 FROM trader WHERE char_id={} AND char_zone_id={} AND char_zone_instance_id={} "
			"AND char_entity_id={} AND active_transaction={} LIMIT 1",
			visual.stall.seller, Zones::BAZAAR, visual.stall.instance, it->first, TraderRepository::OfflineParcel
		), false);
		if (listings.Success() && !listings.RowCount()) {
			Remove(it->first, visual);
			it = stalls.erase(it);
		} else {
			++it;
		}
	}
	for (const auto &stall : OfflineBazaarStall::Parked(database, zone.GetInstanceID())) {
		if (stall.version == zone.GetInstanceVersion()) {
			Restore(stall);
		}
	}
}

void Clear(Zone &zone)
{
	if (zone.GetZoneID() == Zones::BAZAAR) {
		stalls.clear();
		next_update = {};
	}
}

void Send(Client &client)
{
	for (const auto &[entity, visual] : stalls) {
		if (Find(entity, visual)) {
			SendVisual(client, visual.stall, entity, true);
		}
	}
}

uint32_t CounterSeller(Client &buyer, uint32_t entity)
{
	if (!entity || entity > UINT16_MAX || buyer.GetZoneID() != Zones::BAZAAR ||
		buyer.ClientVersion() != EQ::versions::ClientVersion::RoF2 || !RuleB(Bazaar, EnableOfflineTraders)) {
		return 0;
	}
	auto live = entity_list.GetClientByID(entity);
	if (live && live->HasPersistentBazaarStall()) {
		return live->CharacterID();
	}
	const auto restored = stalls.find(entity);
	return restored != stalls.end() && Find(entity, restored->second) ? restored->second.stall.seller : 0;
}

}
