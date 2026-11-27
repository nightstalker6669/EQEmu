#include "guild_banner_store.h"

#include "client.h"
#include "guild_mgr.h"
#include "../common/guild_banner_catalogue.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

extern ZoneGuildManager guild_mgr;

namespace GuildBanner {
namespace {

constexpr auto SelectBanner =
	"SELECT guild_id,zone_id,zone_version,instance_id,planter_char_id,banner_type,"
	"x,y,z,heading,planted_at,upgrade_expires_at,base_expires_at,revision FROM guild_banners";

auto Query(Database& db, const std::string& sql)
{
	if (db.GetStatus() != DBcore::Connected) {
		throw std::runtime_error("Guild banner connection lost");
	}
	auto result = db.QueryDatabase(sql, false);
	if (!result.Success()) {
		throw std::runtime_error("Guild banner database operation failed");
	}
	return result;
}

class Transaction {
public:
	explicit Transaction(Database& db) : m_db(db)
	{
		Query(m_db, "START TRANSACTION");
	}
	~Transaction()
	{
		if (!m_settled && m_db.GetStatus() == DBcore::Connected) {
			m_db.QueryDatabase(std::string("ROLLBACK"), false);
		}
	}
	Result Commit()
	{
		// A lost COMMIT acknowledgement is ambiguous. The caller must reload
		// durable inventory and banner state instead of replaying the debit.
		try {
			Query(m_db, "COMMIT");
			m_settled = true;
			return Result::Success;
		} catch (...) {
			return Result::CommitUncertain;
		}
	}
	Transaction(const Transaction&) = delete;
	Transaction& operator=(const Transaction&) = delete;

private:
	Database& m_db;
	bool m_settled{};
};

bool HasPermission(Database& db, const Client& actor, uint32_t guild_id, GuildAction permission)
{
	if (!guild_id || actor.GuildID() != guild_id) {
		return false;
	}
	// Serialize all deployments for this guild, including the first insert.
	if (Query(db, fmt::format("SELECT id FROM guilds WHERE id={} FOR UPDATE", guild_id)).RowCount() != 1) {
		return false;
	}
	auto rows = Query(db, fmt::format(
		"SELECT rank FROM guild_members WHERE char_id={} AND guild_id={} FOR UPDATE",
		actor.CharacterID(), guild_id
	));
	if (rows.RowCount() != 1 || !rows.begin()[0]) {
		return false;
	}
	const auto rank = std::stoul(rows.begin()[0]);
	return rank >= GUILD_LEADER && rank <= GUILD_MAX_RANK &&
		guild_mgr.CheckPermission(guild_id, static_cast<uint8_t>(rank), permission);
}

Banner DecodeRow(auto& row)
{
	return Banner{
		static_cast<uint32_t>(std::stoul(row[0])),
		static_cast<uint32_t>(std::stoul(row[1])),
		std::stoi(row[2]),
		static_cast<uint32_t>(std::stoul(row[3])),
		static_cast<uint32_t>(std::stoul(row[4])),
		static_cast<uint32_t>(std::stoul(row[5])),
		std::stof(row[6]), std::stof(row[7]), std::stof(row[8]), std::stof(row[9]),
		std::stoull(row[10]), std::stoull(row[11]), std::stoull(row[12]), std::stoull(row[13])
	};
}

std::optional<Banner> Read(Database& db, uint32_t guild_id, bool lock)
{
	auto rows = Query(db, fmt::format("{} WHERE guild_id={}{}", SelectBanner, guild_id, lock ? " FOR UPDATE" : ""));
	return rows.RowCount() == 1 ? std::optional(DecodeRow(rows.begin())) : std::nullopt;
}

bool Valid(const Banner& banner)
{
	return banner.guild_id && banner.planter_char_id && SupportedPublicZone(banner.zone_id) &&
		banner.zone_version == 0 && banner.instance_id == 0 &&
		FindDefinition(banner.banner_type) &&
		std::isfinite(banner.x) && std::isfinite(banner.y) && std::isfinite(banner.z) &&
		std::isfinite(banner.heading) && banner.heading >= 0 && banner.heading < 512;
}

} // namespace

std::optional<Banner> FindActive(Database& db, uint32_t guild_id, uint64_t now)
{
	try {
		auto banner = Read(db, guild_id, false);
		if (banner && banner->revision && banner->base_expires_at > now && Valid(*banner)) {
			if (banner->upgrade_expires_at <= now) {
				banner->banner_type = StandardOfHonor.id;
			}
			return banner;
		}
	} catch (...) {
		// The portal and aura both fail closed when the live state is uncertain.
	}
	return std::nullopt;
}

std::vector<Banner> FindActiveInZone(Database& db, uint32_t zone_id, uint64_t now)
{
	std::vector<Banner> banners;
	try {
		auto rows = Query(db, fmt::format(
			"{} WHERE zone_id={} AND zone_version=0 AND instance_id=0 AND base_expires_at>{}",
			SelectBanner, zone_id, now
		));
		for (auto row : rows) {
			auto banner = DecodeRow(row);
			if (!banner.revision || !Valid(banner)) {
				continue;
			}
			if (banner.upgrade_expires_at <= now) {
				banner.banner_type = StandardOfHonor.id;
			}
			banners.push_back(banner);
		}
	} catch (...) {
		banners.clear();
	}
	return banners;
}

void RefreshBaseLifetime(Database& db, const Banner& banner, uint32_t present_member, uint64_t now)
{
	constexpr uint64_t UpdateInterval = 30;
	if (!present_member || !banner.revision || now > UINT64_MAX - BannerLifetime ||
		banner.base_expires_at >= now + BannerLifetime - UpdateInterval) {
		return;
	}
	db.QueryDatabase(fmt::format(
		"UPDATE guild_banners SET base_expires_at=GREATEST(base_expires_at,{}) "
		"WHERE guild_id={} AND revision={} AND zone_id={} AND zone_version=0 AND instance_id=0 "
		"AND base_expires_at>{} AND EXISTS "
		"(SELECT 1 FROM guild_members WHERE char_id={} AND guild_id={})",
		now + BannerLifetime, banner.guild_id, banner.revision, banner.zone_id, now,
		present_member, banner.guild_id
	), false);
}

Result Plant(Database& db, const Client& actor, Banner banner,
	std::span<const uint32_t> local_members, uint64_t now, const MaterialDebit& debit)
{
	const auto* definition = FindDefinition(banner.banner_type);
	if (!Valid(banner) || actor.CharacterID() != banner.planter_char_id ||
		banner.planted_at != now || now > UINT64_MAX - BannerLifetime ||
		banner.base_expires_at != now + BannerLifetime ||
		banner.upgrade_expires_at != (definition->spell_id ? now + BannerLifetime : 0) ||
		bool(definition->material_item) != bool(debit)) {
		return Result::Invalid;
	}
	const auto minimum_members = RuleI(Guild, BannerMinimumMembers);
	if (!HasMinimumMembers(local_members.size(), minimum_members) ||
		std::find(local_members.begin(), local_members.end(), actor.CharacterID()) == local_members.end()) {
		return Result::InsufficientMembers;
	}

	try {
		Transaction transaction(db);
		if (!HasPermission(db, actor, banner.guild_id, GUILD_ACTION_BANNER_PLANT)) {
			return Result::PermissionDenied;
		}
		auto prior = Read(db, banner.guild_id, true);
		if (prior && prior->base_expires_at > now) {
			return Result::AlreadyPlanted;
		}
		if (prior && prior->revision == UINT64_MAX) {
			return Result::Failed;
		}
		std::string ids;
		for (const auto id : local_members) {
			if (!ids.empty()) {
				ids += ',';
			}
			ids += std::to_string(id);
		}
		const auto members = Query(db, fmt::format(
			"SELECT char_id FROM guild_members WHERE guild_id={} AND char_id IN ({}) FOR UPDATE",
			banner.guild_id, ids
		));
		if (!HasMinimumMembers(members.RowCount(), minimum_members)) {
			return Result::InsufficientMembers;
		}
		if (debit && !debit(db)) {
			return Result::MissingMaterials;
		}
		banner.revision = prior ? prior->revision + 1 : 1;
		Query(db, fmt::format(
			"INSERT INTO guild_banners "
			"(guild_id,zone_id,zone_version,instance_id,planter_char_id,banner_type,x,y,z,heading,"
			"planted_at,upgrade_expires_at,base_expires_at,revision) "
			"VALUES ({},{},{},{},{},{},{:.9g},{:.9g},{:.9g},{:.9g},{},{},{},{}) "
			"ON DUPLICATE KEY UPDATE zone_id=VALUES(zone_id),zone_version=VALUES(zone_version),"
			"instance_id=VALUES(instance_id),planter_char_id=VALUES(planter_char_id),"
			"banner_type=VALUES(banner_type),x=VALUES(x),y=VALUES(y),z=VALUES(z),heading=VALUES(heading),"
			"planted_at=VALUES(planted_at),upgrade_expires_at=VALUES(upgrade_expires_at),"
			"base_expires_at=VALUES(base_expires_at),revision=VALUES(revision)",
			banner.guild_id, banner.zone_id, banner.zone_version, banner.instance_id,
			banner.planter_char_id, banner.banner_type, banner.x, banner.y, banner.z, banner.heading,
			banner.planted_at, banner.upgrade_expires_at, banner.base_expires_at, banner.revision
		));
		return transaction.Commit();
	} catch (...) {
		return Result::Failed;
	}
}

Result Remove(Database& db, const Client& actor, uint32_t guild_id, uint64_t revision)
{
	if (!revision || revision == UINT64_MAX) {
		return Result::Invalid;
	}
	try {
		Transaction transaction(db);
		if (!HasPermission(db, actor, guild_id, GUILD_ACTION_BANNER_REMOVE)) {
			return Result::PermissionDenied;
		}
		auto rows = Query(db, fmt::format(
			"UPDATE guild_banners SET base_expires_at=0,upgrade_expires_at=0,revision=revision+1 "
			"WHERE guild_id={} AND revision={} AND base_expires_at>UNIX_TIMESTAMP()", guild_id, revision
		));
		if (rows.RowsAffected() != 1) {
			return Result::Invalid;
		}
		return transaction.Commit();
	} catch (...) {
		return Result::Failed;
	}
}

} // namespace GuildBanner
