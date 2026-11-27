#ifndef EQEMU_GUILD_BANNER_WIRE_H
#define EQEMU_GUILD_BANNER_WIRE_H

#include <algorithm>
#include <bit>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace GuildBanner::RoF2 {

// Application payload sizes exclude the two-byte RoF2 opcode.
constexpr size_t ListRequestSize = 141;
constexpr size_t PlantRequestSize = 87;
constexpr size_t RemoveRequestSize = 12;
constexpr size_t ListHeaderSize = 16;
constexpr size_t ListEntrySize = 137;
constexpr size_t StatusSize = 188;
constexpr size_t MaximumListEntries = 1023;

using Bytes = std::vector<uint8_t>;

struct ListRequest {
	uint32_t guild_hint;
	bool usable_only;
	uint32_t filter;
};

struct PlantRequest {
	uint32_t banner_id;
	float y;
	float x;
	float z;
	bool new_selection;
	bool save_settings;
	uint8_t additional_action;
};

struct ListEntry {
	uint32_t id;
	std::string_view name;
	uint32_t description_id;
	bool usable;
};

inline uint32_t Read32(std::span<const uint8_t> bytes, size_t offset)
{
	return uint32_t(bytes[offset]) |
		(uint32_t(bytes[offset + 1]) << 8) |
		(uint32_t(bytes[offset + 2]) << 16) |
		(uint32_t(bytes[offset + 3]) << 24);
}

inline void Write32(Bytes& bytes, size_t offset, uint32_t value)
{
	for (unsigned byte = 0; byte < 4; ++byte) {
		bytes[offset + byte] = static_cast<uint8_t>(value >> (byte * 8));
	}
}

inline std::optional<ListRequest> DecodeListRequest(std::span<const uint8_t> bytes)
{
	if (bytes.size() != ListRequestSize || bytes[132] > 1 || Read32(bytes, 133) > 32) {
		return std::nullopt;
	}
	return ListRequest{Read32(bytes, 0), bytes[132] != 0, Read32(bytes, 137)};
}

inline std::optional<PlantRequest> DecodePlantRequest(std::span<const uint8_t> bytes)
{
	if (bytes.size() != PlantRequestSize || bytes[4] != 0 || bytes[80] > 1 || bytes[81] > 1) {
		return std::nullopt;
	}
	return PlantRequest{
		Read32(bytes, 0),
		std::bit_cast<float>(Read32(bytes, 68)),
		std::bit_cast<float>(Read32(bytes, 72)),
		std::bit_cast<float>(Read32(bytes, 76)),
		bytes[80] != 0, bytes[81] != 0, bytes[86]
	};
}

inline std::optional<uint32_t> DecodeRemoveGuildHint(std::span<const uint8_t> bytes)
{
	if (bytes.size() != RemoveRequestSize || Read32(bytes, 0) != UINT32_MAX) {
		return std::nullopt;
	}
	return Read32(bytes, 4);
}

inline Bytes EncodeList(
	uint32_t guild_id,
	std::span<const ListEntry> entries,
	uint32_t emblem_choices,
	uint32_t background_choices
)
{
	const auto count = std::min(entries.size(), MaximumListEntries);
	Bytes bytes(ListHeaderSize + count * ListEntrySize, 0);
	Write32(bytes, 0, guild_id);
	Write32(bytes, 4, static_cast<uint32_t>(count));
	Write32(bytes, 8, emblem_choices);
	Write32(bytes, 12, background_choices);
	for (size_t i = 0; i < count; ++i) {
		const auto& entry = entries[i];
		const size_t offset = ListHeaderSize + i * ListEntrySize;
		Write32(bytes, offset, entry.id);
		const auto length = std::min({entry.name.size(), entry.name.find('\0'), size_t{127}});
		std::copy_n(entry.name.begin(), length, bytes.begin() + offset + 4);
		bytes[offset + 132] = entry.usable;
		Write32(bytes, offset + 133, entry.description_id);
	}
	return bytes;
}

inline Bytes EncodeStatus(
	uint32_t guild_id,
	const std::optional<ListEntry>& active,
	uint32_t seconds_remaining
)
{
	Bytes bytes(StatusSize, 0);
	Write32(bytes, 0, guild_id);
	// Type 2 replaces the status snapshot without requesting the list again.
	Write32(bytes, 4, 2);
	Write32(bytes, 12, active ? active->id : UINT32_MAX);
	Write32(bytes, 16, active ? active->description_id : 0);
	if (active) {
		// May RoF2 subtracts elapsed milliseconds from the countdown at +20.
		Write32(bytes, 20, std::min(seconds_remaining, uint32_t{INT32_MAX / 1000}) * 1000);
		const auto length = std::min({active->name.size(), active->name.find('\0'), size_t{127}});
		std::copy_n(active->name.begin(), length, bytes.begin() + 24);
	}
	Write32(bytes, 168, active ? seconds_remaining : UINT32_MAX);
	return bytes;
}

} // namespace GuildBanner::RoF2

#endif
