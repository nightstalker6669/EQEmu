#pragma once

#include "common/eq_packet_structs.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <vector>

namespace EQ::patches {
struct CharacterSelectLayout {
    std::vector<size_t> name_lengths;
    size_t total_length;
};

// Validate the fixed-size server entries before reading names or allocating the
// variable-size client packet. Each wire entry already includes its NUL byte.
inline std::optional<CharacterSelectLayout> GetCharacterSelectLayout(
    const unsigned char *buffer, size_t size, size_t limit,
    size_t wire_header_size, size_t wire_entry_size)
{
    if (size < sizeof(CharacterSelect_Struct)) {
        return std::nullopt;
    }
    const auto *header = reinterpret_cast<const CharacterSelect_Struct *>(buffer);
    if (header->CharCount > (size - sizeof(CharacterSelect_Struct)) / sizeof(CharacterSelectEntry_Struct)) {
        return std::nullopt;
    }
    constexpr size_t maximum = std::numeric_limits<uint32_t>::max();
    if (wire_header_size > maximum) {
        return std::nullopt;
    }
    CharacterSelectLayout layout{{}, wire_header_size};
    const size_t count = std::min<size_t>(header->CharCount, limit);
    const auto *entries = reinterpret_cast<const CharacterSelectEntry_Struct *>(buffer + sizeof(CharacterSelect_Struct));
    for (size_t i = 0; i < count; ++i) {
        const auto *end = static_cast<const char *>(std::memchr(entries[i].Name, 0, sizeof(entries[i].Name)));
        if (!end) {
            return std::nullopt;
        }
        const size_t length = end - entries[i].Name;
        if (wire_entry_size > maximum - layout.total_length ||
            length > maximum - layout.total_length - wire_entry_size) {
            return std::nullopt;
        }
        layout.name_lengths.push_back(length);
        layout.total_length += wire_entry_size + length;
    }
    return layout;
}
} // namespace EQ::patches
