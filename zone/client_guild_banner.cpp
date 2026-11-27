#include "client.h"

#include "entity.h"
#include "guild_banner_store.h"
#include "guild_mgr.h"
#include "inventory_session_owner.h"
#include "map.h"
#include "water_map.h"
#include "zone.h"
#include "zonedb.h"
#include "../common/eqemu_config.h"
#include "../common/fellowship/campsite_materials.h"
#include "../common/guild_banner_catalogue.h"
#include "../common/guild_banner_wire.h"
#include "../common/housing/inventory_projection.h"
#include "../common/housing/inventory_row_db.h"

#include <array>
#include <cmath>
#include <ctime>

namespace {

namespace Wire = GuildBanner::RoF2;
using GuildBanner::Definition;
using GuildBanner::Result;

bool Ready(Client& client)
{
	return zone && client.ClientVersion() == EQ::versions::ClientVersion::RoF2 &&
		client.Connected() && client.ClientDataLoaded() && client.IsInAGuild() &&
		(client.GetPP().expansions & EQ::expansions::bitTBS);
}

std::span<const uint8_t> Payload(const EQApplicationPacket* packet)
{
	return packet ? std::span<const uint8_t>(packet->pBuffer, packet->size) : std::span<const uint8_t>{};
}

void Send(Client& client, EmuOpcode opcode, const Wire::Bytes& bytes)
{
	EQApplicationPacket packet(opcode, bytes.size());
	std::copy(bytes.begin(), bytes.end(), packet.pBuffer);
	client.QueuePacket(&packet);
}

void SendStatus(Client& client, const std::optional<GuildBanner::Banner>& banner, uint64_t now)
{
	std::optional<Wire::ListEntry> active;
	if (banner) {
		if (const auto* definition = GuildBanner::FindDefinition(banner->banner_type)) {
			active = Wire::ListEntry{definition->id, definition->name, definition->description_id, true};
		}
	}
	const uint32_t remaining = active ?
		static_cast<uint32_t>(std::min<uint64_t>(banner->base_expires_at - now, UINT32_MAX - 1)) : 0;
	Send(client, OP_GuildBannerStatus, Wire::EncodeStatus(client.GuildID(), active, remaining));
}

struct Material {
	int16_t slot{};
	EQ::ItemInstance* item{};
	int32_t serial{};
	int16_t charges{};
	uint32_t id{};
};
using Materials = std::array<Material, 2>;

std::optional<Materials> StageMaterials(Client& client, const Definition& definition)
{
	const std::array<uint32_t, 2> recipe{definition.material_item, definition.dye_item};
	Materials materials{};
	for (size_t i = 0; i < materials.size(); ++i) {
		const auto slot = client.GetInv().HasItem(recipe[i], 1, invWherePersonal);
		auto* item = client.GetInv().GetItem(slot);
		if (slot == INVALID_INDEX || !item || !item->GetItem() || item->GetID() != recipe[i] ||
			item->GetItem()->ItemType != 58 || !item->IsStackable() ||
			item->GetCharges() < 1 || item->GetEvolveUniqueID()) {
			return std::nullopt;
		}
		materials[i] = Material{slot, item, item->GetSerialNumber(), item->GetCharges(), recipe[i]};
	}
	return materials[0].slot == materials[1].slot ? std::nullopt : std::optional(materials);
}

bool SameMaterials(Client& client, const Materials& materials)
{
	return std::all_of(materials.begin(), materials.end(), [&](const Material& material) {
		const auto* current = client.GetInv().GetItem(material.slot);
		return current && current == material.item && current->GetID() == material.id &&
			current->GetSerialNumber() == material.serial && current->GetCharges() == material.charges;
	});
}

void RequireReload(Client& client)
{
	client.RejectInventorySessionAdmission();
	client.Kick("Guild banner inventory outcome requires authoritative reload");
}

bool Near(const glm::vec3& point, const glm::vec3& other, float radius)
{
	const auto distance = point - other;
	return glm::dot(distance, distance) <= radius * radius;
}

bool CanPlantHere(Client& client, glm::vec3 position, float& floor)
{
	if (!GuildBanner::SupportedPublicZone(zone->GetZoneID()) || zone->GetInstanceID() ||
		zone->GetInstanceVersion() || !zone->zonemap ||
		!zone->CanCastOutdoor() || !zone->CanDoCombat() || zone->IsPVPZone() ||
		zone->IsSpellBlocked(1771, position) || zone->IsSpellBlocked(13260, position) ||
		client.GetHP() <= 0 || client.GetBoatID()) {
		return false;
	}
	if (zone->watermap && (zone->watermap->InWater(position) || zone->watermap->InLava(position))) {
		return false;
	}
	floor = zone->zonemap->FindBestZ(position, nullptr);
	// The stock May client compares native floor and player Z against 10 units.
	return std::isfinite(floor) && floor != BEST_Z_INVALID && std::abs(position.z - floor) < 10.0f;
}

std::vector<uint32_t> LocalQuorum(Client& actor, const glm::vec3& position)
{
	std::vector<uint32_t> members;
	for (const auto& [entity, client] : entity_list.GetClientList()) {
		if (client && client->Connected() && client->ClientDataLoaded() && client->GetHP() > 0 &&
			client->GuildID() == actor.GuildID() &&
			Near(position, glm::vec3(client->GetPosition()), GuildBanner::QuorumRadius)) {
			members.push_back(client->CharacterID());
		}
	}
	return members;
}

Result PlantWithMaterials(
	Database& db, Client& client, const Definition& definition,
	const GuildBanner::Banner& banner, std::span<const uint32_t> members, uint64_t now
)
{
	auto materials = StageMaterials(client, definition);
	if (!materials) {
		return Result::MissingMaterials;
	}
	const auto ticket = client.InventorySessionReleaseTicket();
	const auto lease = ticket ? ticket->SnapshotFor(client) : std::nullopt;
	if (!lease || !ticket->HeldFor(client) || !client.HasInventorySessionReservation()) {
		return Result::Failed;
	}

	auto result = GuildBanner::Plant(db, client, banner, members, now, [&](Database& transaction) {
		if (!ticket->HeldFor(client) || client.InventorySessionReleaseTicket() != ticket ||
			!SameMaterials(client, *materials)) {
			return false;
		}
		const auto current = ticket->SnapshotFor(client);
		if (!current || current->epoch != lease->epoch || current->token != lease->token) {
			return false;
		}
		std::array<Housing::LiveItems::InventoryRow, 2> rows;
		for (size_t i = 0; i < rows.size(); ++i) {
			auto selected = transaction.QueryDatabase(fmt::format(
				"SELECT {} FROM inventory WHERE character_id={} AND slot_id={}",
				Housing::InventoryRowDB::Columns, client.CharacterID(), (*materials)[i].slot
			), false);
			if (!selected.Success() || selected.RowCount() != 1) {
				return false;
			}
			rows[i] = Housing::InventoryRowDB::Decode(selected.begin());
			if (!Housing::InventoryProjection::Matches(rows[i], *(*materials)[i].item)) {
				return false;
			}
		}
		return GuildBanner::ConsumeMaterialsUnderTransaction(transaction, definition.id, *lease, rows);
	});
	if (result == Result::CommitUncertain ||
		(result == Result::Success && (!ticket->HeldFor(client) || !SameMaterials(client, *materials)))) {
		RequireReload(client);
		return Result::CommitUncertain;
	}
	if (result == Result::Success) {
		for (const auto& material : *materials) {
			client.DeleteItemInInventory(material.slot, 1, true, false);
		}
	}
	return result;
}

bool Connect(Database& database)
{
	const auto* config = EQEmuConfig::get();
	return config && database.Connect(
		config->DatabaseHost, config->DatabaseUsername, config->DatabasePassword,
		config->DatabaseDB, config->DatabasePort, "guild-banner"
	);
}

} // namespace

void Client::Handle_OP_GuildBannerListRequest(const EQApplicationPacket* packet)
{
	const auto request = Wire::DecodeListRequest(Payload(packet));
	if (!Ready(*this) || !request) {
		return;
	}
	const auto now = static_cast<uint64_t>(std::time(nullptr));
	const auto current = GuildBanner::FindActive(database, GuildID(), now);
	std::vector<Wire::ListEntry> entries;
	auto append = [&](const Definition& definition) {
		if (request->filter != UINT32_MAX && !(request->filter & (1u << definition.category))) {
			return;
		}
		const bool usable = !definition.material_item || StageMaterials(*this, definition).has_value();
		if (!request->usable_only || usable) {
			entries.push_back({definition.id, definition.name, definition.description_id, usable});
		}
	};
	append(GuildBanner::StandardOfHonor);
	for (const auto& definition : GuildBanner::UpgradedStandards) {
		append(definition);
	}
	SendStatus(*this, current, now);
	// The model preview requires at least one available base and emblem choice.
	Send(*this, OP_GuildBannerList, Wire::EncodeList(GuildID(), entries, 1, 1));
}

void Client::Handle_OP_GuildBannerPlant(const EQApplicationPacket* packet)
{
	const auto request = Wire::DecodePlantRequest(Payload(packet));
	if (!Ready(*this) || !request || !request->new_selection || request->save_settings || request->additional_action) {
		return;
	}
	if (!trade || trade->state != TradeNone || trade->With()) {
		return;
	}
	const auto* definition = GuildBanner::FindDefinition(request->banner_id);
	const glm::vec3 position(GetPosition());
	float floor{};
	if (!definition || !Near(position, {request->x, request->y, request->z}, 10.0f) ||
		!CanPlantHere(*this, position, floor)) {
		Message(Chat::Red, "Cannot place a guild banner at this location.");
		return;
	}
	const auto members = LocalQuorum(*this, position);
	const auto minimum_members = RuleI(Guild, BannerMinimumMembers);
	if (!GuildBanner::HasMinimumMembers(members.size(), minimum_members)) {
		Message(Chat::Red, "%u nearby living guild members are required to plant a banner.",
			GuildBanner::RequiredMembers(minimum_members));
		return;
	}
	Database exclusive;
	if (!Connect(exclusive)) {
		Message(Chat::Red, "The guild banner service is temporarily unavailable.");
		return;
	}
	const auto now = static_cast<uint64_t>(std::time(nullptr));
	const GuildBanner::Banner banner{
		GuildID(), zone->GetZoneID(), zone->GetInstanceVersion(), zone->GetInstanceID(),
		CharacterID(), definition->id, position.x, position.y, floor, GetHeading(), now,
		definition->spell_id ? now + GuildBanner::BannerLifetime : 0,
		now + GuildBanner::BannerLifetime
	};
	const auto result = definition->material_item ?
		PlantWithMaterials(exclusive, *this, *definition, banner, members, now) :
		GuildBanner::Plant(exclusive, *this, banner, members, now);
	switch (result) {
	case Result::Success:
		SendStatus(*this, GuildBanner::FindActive(database, GuildID(), now), now);
		Message(Chat::Yellow, "You have planted %s.", definition->name.data());
		break;
	case Result::PermissionDenied:
		Message(Chat::Red, "Your guild rank cannot plant a guild banner.");
		break;
	case Result::InsufficientMembers:
		Message(Chat::Red, "%u nearby living guild members are required to plant a banner.",
			GuildBanner::RequiredMembers(minimum_members));
		break;
	case Result::AlreadyPlanted:
		Message(Chat::Red, "Your guild already has an active banner.");
		break;
	case Result::MissingMaterials:
		Message(Chat::Red, "You need the banner material and dye in your personal inventory.");
		break;
	case Result::CommitUncertain:
		break;
	default:
		Message(Chat::Red, "The guild banner could not be planted.");
		break;
	}
}

void Client::Handle_OP_GuildBannerRemove(const EQApplicationPacket* packet)
{
	auto request = Wire::DecodeRemoveGuildHint(Payload(packet));
	if (!Ready(*this) || !request || *request != GuildID()) {
		return;
	}
	const auto now = static_cast<uint64_t>(std::time(nullptr));
	const auto current = GuildBanner::FindActive(database, GuildID(), now);
	Database exclusive;
	if (!current || !Connect(exclusive)) {
		return;
	}
	const auto result = GuildBanner::Remove(exclusive, *this, GuildID(), current->revision);
	if (result == Result::Success) {
		SendStatus(*this, std::nullopt, now);
		Message(Chat::Yellow, "Your guild banner has been pulled up.");
	} else if (result == Result::PermissionDenied) {
		Message(Chat::Red, "Your guild rank cannot remove a guild banner.");
	}
}
