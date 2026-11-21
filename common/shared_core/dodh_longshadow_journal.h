#pragma once

#include "dodh_longshadow_custody.h"
#include <string>
#include <string_view>

namespace EQEmu::SharedCore::DodhLongshadowCustody {
inline constexpr std::string_view JournalPrefix{"DODH\x01\x04", 6};

struct JournalRecord {
    uint64_t occurrence;
    uint32_t accepted_time;
    uint32_t task;
    uint32_t activity;
    uint32_t instance;
    uint32_t guid;
};

inline std::string EncodeJournal(const JournalRecord& record)
{
    std::string out(JournalPrefix);
    const auto append = [&](uint64_t value, unsigned width) {
        for (unsigned i = 0; i < width; ++i) out += char(uint8_t(value >> (8 * i)));
    };
    append(record.occurrence, 8);
    append(record.accepted_time, 4);
    append(record.task, 4);
    append(record.activity, 4);
    append(record.instance, 4);
    append(record.guid, 4);
    return out;
}

inline std::optional<JournalRecord> DecodeJournal(std::string_view bytes)
{
    if (bytes.size() != JournalPrefix.size() + 28 ||
        bytes.substr(0, JournalPrefix.size()) != JournalPrefix) return std::nullopt;
    std::size_t offset = JournalPrefix.size();
    const auto read = [&](unsigned width) {
        uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i)
            value |= uint64_t(uint8_t(bytes[offset++])) << (8 * i);
        return value;
    };
    const JournalRecord record{read(8), uint32_t(read(4)), uint32_t(read(4)),
        uint32_t(read(4)), uint32_t(read(4)), uint32_t(read(4))};
    const auto* route = FindRoute(record.task, record.activity);
    if (!route || (!TaskOnly(route->effect) && !SceneActor(route->effect)) || !record.occurrence || !record.accepted_time ||
        !record.instance || !record.guid || record.guid > INT32_MAX) return std::nullopt;
    return record;
}
}
