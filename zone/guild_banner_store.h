#ifndef EQEMU_GUILD_BANNER_STORE_H
#define EQEMU_GUILD_BANNER_STORE_H

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

class Client;
class Database;

namespace GuildBanner {

struct Banner {
	uint32_t guild_id{};
	uint32_t zone_id{};
	int32_t zone_version{};
	uint32_t instance_id{};
	uint32_t planter_char_id{};
	uint32_t banner_type{};
	float x{}, y{}, z{}, heading{};
	uint64_t planted_at{};
	uint64_t upgrade_expires_at{};
	uint64_t base_expires_at{};
	uint64_t revision{};
};

enum class Result {
	Success,
	PermissionDenied,
	Invalid,
	AlreadyPlanted,
	InsufficientMembers,
	MissingMaterials,
	Failed,
	CommitUncertain
};

using MaterialDebit = std::function<bool(Database&)>;

std::optional<Banner> FindActive(Database& db, uint32_t guild_id, uint64_t now);
std::vector<Banner> FindActiveInZone(Database& db, uint32_t zone_id, uint64_t now);
void RefreshBaseLifetime(Database& db, const Banner& banner, uint32_t present_member, uint64_t now);
Result Plant(
	Database& db, const Client& actor, Banner banner,
	std::span<const uint32_t> local_members, uint64_t now,
	const MaterialDebit& debit = {}
);
Result Remove(Database& db, const Client& actor, uint32_t guild_id, uint64_t revision);

} // namespace GuildBanner

#endif
