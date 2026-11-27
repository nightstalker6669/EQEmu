#ifndef EQEMU_GUILD_BANNER_CATALOGUE_H
#define EQEMU_GUILD_BANNER_CATALOGUE_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace GuildBanner {

constexpr uint32_t TravelStone = 88737;
constexpr uint32_t DefaultRequiredMembers = 12;
constexpr float QuorumRadius = 150.0f;
constexpr uint64_t BannerLifetime = 3600;

inline constexpr uint32_t RequiredMembers(int rule_value)
{
	if (rule_value < 2 || rule_value > 72) {
		return DefaultRequiredMembers;
	}
	return static_cast<uint32_t>(rule_value);
}

inline constexpr bool HasMinimumMembers(std::size_t member_count, int rule_value)
{
	return member_count >= RequiredMembers(rule_value);
}

static_assert(!HasMinimumMembers(11, DefaultRequiredMembers));
static_assert(HasMinimumMembers(12, DefaultRequiredMembers));
static_assert(!HasMinimumMembers(1, 2) && HasMinimumMembers(2, 2));

// These public layouts have matching client banner assets and reviewed
// ordinary entry. Do not turn a missing key or scripted entry into a portal.
inline constexpr bool SupportedPublicZone(uint32_t zone_id)
{
	return zone_id == 423 || zone_id == 443;
}

struct Definition {
	uint32_t id;
	std::string_view name;
	uint32_t description_id;
	uint8_t category;
	uint32_t material_item;
	uint32_t dye_item;
	uint16_t spell_id;
	uint8_t caster_level;
};

// The native list treats the row ID as opaque and returns it when selected.
// Use the shipped description IDs as stable server catalogue keys.
inline constexpr Definition StandardOfHonor{40778, "Standard of Honor", 40778, 1, 0, 0, 0, 0};

inline constexpr std::array<Definition, 556> UpgradedStandards{{
#include "guild_banner_catalogue_data.inc"
}};

inline constexpr const Definition* FindDefinition(uint32_t id)
{
	if (id == StandardOfHonor.id) {
		return &StandardOfHonor;
	}
	for (const auto& entry : UpgradedStandards) {
		if (entry.id == id) {
			return &entry;
		}
	}
	return nullptr;
}

} // namespace GuildBanner

#endif
