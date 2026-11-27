#include "guild_banner_portal.h"

#include "client.h"
#include "entity.h"
#include "npc.h"
#include "zone.h"
#include "zonedb.h"
#include "../common/data_bucket.h"
#include "../common/guild_banner_catalogue.h"
#include "../common/zone_store.h"

#include <array>
#include <charconv>
#include <cmath>
#include <ctime>

namespace GuildBanner {
namespace {

constexpr char BasicOfferVariable[] = "guild_banner_basic_offer";
constexpr char BasicEntryVariable[] = "guild_banner_basic_entry";
constexpr uint32_t BasicHall = 345;
constexpr uint32_t PorterType = 345005;
constexpr uint32_t PorterSpawn = 76577;
constexpr uint32_t MerchantType = 345004;
constexpr uint32_t MerchantSpawn = 57311;
constexpr float PadRadius = 10.0f;
constexpr uint32_t OfferSeconds = 30;

uint64_t Now()
{
	return static_cast<uint64_t>(std::time(nullptr));
}

std::optional<uint64_t> ParseUnsigned(const std::string& value)
{
	uint64_t result{};
	const char* begin = value.data();
	const char* end = begin + value.size();
	const auto parsed = std::from_chars(begin, end, result);
	return parsed.ec == std::errc{} && parsed.ptr == end ? std::optional(result) : std::nullopt;
}

bool IsBasicHall(Client& client)
{
	if (!zone || zone->GetZoneID() != BasicHall || zone->GetInstanceVersion() != 1 ||
		!zone->GetInstanceID() || !client.IsInAGuild()) {
		return false;
	}
	const auto instance = ParseUnsigned(DataBucket::GetData(
		&database, fmt::format("guild-hall-instance-{}", client.GuildID())
	));
	return instance && *instance == zone->GetInstanceID() &&
		database.VerifyInstanceAlive(zone->GetInstanceID(), client.CharacterID()) &&
		database.VerifyZoneInstance(BasicHall, zone->GetInstanceID());
}

bool IsBasicPorter(Mob* porter)
{
	return porter && porter->IsNPC() && porter->GetNPCTypeID() == PorterType &&
		porter->CastToNPC()->GetSpawnPointID() == PorterSpawn;
}

bool AtPad(Client& client, Mob* porter)
{
	if (!IsBasicPorter(porter)) {
		return false;
	}
	const auto delta = client.GetPosition() - porter->GetPosition();
	// Match the original Porter's square approach on the portal deck.
	return std::abs(delta.x) <= PadRadius && std::abs(delta.y) <= PadRadius &&
		std::abs(delta.z) <= PadRadius;
}

bool CurrentMember(Client& client, uint32_t guild_id)
{
	if (!client.IsInGuild(guild_id)) {
		return false;
	}
	auto rows = database.QueryDatabase(fmt::format(
		"SELECT char_id FROM guild_members WHERE char_id={} AND guild_id={}",
		client.CharacterID(), guild_id
	), false);
	return rows.Success() && rows.RowCount() == 1;
}

bool PortalLocationAllowed(const Banner& banner)
{
	const auto* layout = GetZoneVersionWithFallback(banner.zone_id, 0);
	if (!layout || layout->version != 0 || !layout->castoutdoor || !layout->cancombat ||
		layout->insttype != 0 || !SupportedPublicZone(banner.zone_id)) {
		return false;
	}
	// A checked zone row can acquire a new regional spell restriction while
	// a banner is planted. Honor it when offering and when confirming travel.
	const auto rows = content_db.QueryDatabase(fmt::format(
		"SELECT id FROM blocked_spells WHERE zoneid={} AND spellid IN (0,1771,13260,16531,31597) "
		"AND (type=1 OR (type=2 AND ABS(x-({:.9g}))<=x_diff AND ABS(y-({:.9g}))<=y_diff "
		"AND ABS(z-({:.9g}))<=z_diff)) LIMIT 1",
		banner.zone_id, banner.x, banner.y, banner.z
	), false);
	return rows.Success() && rows.RowCount() == 0;
}

} // namespace

std::optional<Banner> PortalDestination(Client& client, uint32_t guild_id)
{
	if (!zone || !client.Connected() ||
		!client.ClientDataLoaded() || client.GetHP() <= 0 ||
		!(client.GetPP().expansions & EQ::expansions::bitTBS) || !CurrentMember(client, guild_id)) {
		return std::nullopt;
	}
	auto banner = FindActive(database, guild_id, Now());
	if (!banner || banner->instance_id || banner->zone_version || !PortalLocationAllowed(*banner)) {
		return std::nullopt;
	}
	const auto destination = ZoneName(banner->zone_id);
	if (!destination || !*destination || !client.CanEnterZone(destination, 0)) {
		return std::nullopt;
	}
	return banner;
}

bool TravelFromPortal(Client& client, uint32_t guild_id, uint64_t revision)
{
	const auto banner = PortalDestination(client, guild_id);
	if (!banner || !revision || banner->revision != revision) {
		client.Message(Chat::Red, "The current guild banner cannot be teleported to.");
		return false;
	}
	client.MovePC(banner->zone_id, 0, banner->x, banner->y, banner->z + 3.75f, banner->heading);
	return true;
}

bool CanAlignBasicPortal(Client& client, Mob* merchant)
{
	return IsBasicHall(client) && merchant && merchant->IsNPC() &&
		merchant->GetNPCTypeID() == MerchantType &&
		merchant->CastToNPC()->GetSpawnPointID() == MerchantSpawn &&
		PortalDestination(client, client.GuildID()).has_value();
}

bool OfferBasicPortal(Client& client, Mob* porter)
{
	if (!zone || zone->GetZoneID() != BasicHall || !IsBasicPorter(porter)) {
		return false;
	}
	// EVENT_ENTER runs inside ProcessMove before the accepted pose is published.
	client.SetEntityVariable(BasicEntryVariable, std::to_string(porter->GetID()));
	return true;
}

void ProcessBasicPortalEntry(Client& client)
{
	if (!zone || zone->GetZoneID() != BasicHall) {
		return;
	}
	const auto queued = client.GetEntityVariable(BasicEntryVariable);
	if (queued.empty()) {
		return;
	}
	client.DeleteEntityVariable(BasicEntryVariable);
	const auto entity = ParseUnsigned(queued);
	auto* porter = entity && *entity > 0 && *entity <= UINT16_MAX ?
		entity_list.GetMob(static_cast<uint16_t>(*entity)) : nullptr;
	if (!IsBasicHall(client) || !AtPad(client, porter) ||
		DataBucket::GetData(&database, fmt::format("guild_portal-{}", client.GuildID())) !=
			std::to_string(TravelStone)) {
		return;
	}
	const auto banner = PortalDestination(client, client.GuildID());
	if (!banner) {
		client.Message(Chat::Red, "The current guild banner cannot be teleported to.");
		return;
	}
	const auto expiry = Now() + OfferSeconds;
	client.SetEntityVariable(BasicOfferVariable, fmt::format(
		"{}:{}:{}:{}:{}", client.GuildID(), zone->GetInstanceID(), porter->GetID(), banner->revision, expiry
	));
	client.SendFullPopup("Guild Hall Portal", "Do you wish to travel to your guild banner?",
		TravelStone, TravelStone, 1, OfferSeconds, "Yes", "No", 0);
}

bool HandleBasicPopup(Client& client, const EQApplicationPacket* packet)
{
	if (!packet || packet->size != sizeof(PopupResponse_Struct)) {
		return false;
	}
	const auto* reply = reinterpret_cast<const PopupResponse_Struct*>(packet->pBuffer);
	if (reply->popupid != TravelStone) {
		return false;
	}
	const std::string token = client.GetEntityVariable(BasicOfferVariable);
	client.DeleteEntityVariable(BasicOfferVariable);
	if (token.empty() || reply->unknown0000 != 0 || !IsBasicHall(client)) {
		return true;
	}
	std::array<uint64_t, 5> fields{};
	size_t from = 0;
	for (size_t i = 0; i < fields.size(); ++i) {
		const auto to = token.find(':', from);
		if ((i + 1 < fields.size()) != (to != std::string::npos)) {
			return true;
		}
		const auto value = ParseUnsigned(token.substr(from, to == std::string::npos ? to : to - from));
		if (!value) {
			return true;
		}
		fields[i] = *value;
		from = to + 1;
	}
	if (fields[0] == client.GuildID() && fields[1] == zone->GetInstanceID() &&
		fields[2] <= UINT16_MAX && fields[4] > Now() &&
		AtPad(client, entity_list.GetMob(static_cast<uint16_t>(fields[2]))) &&
		DataBucket::GetData(&database, fmt::format("guild_portal-{}", client.GuildID())) ==
			std::to_string(TravelStone)) {
		TravelFromPortal(client, client.GuildID(), fields[3]);
	}
	return true;
}

} // namespace GuildBanner
