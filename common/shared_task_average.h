#pragma once

#include "common/shared_task_admission.h"
#include <algorithm>

namespace SharedTaskAdmission {

struct AveragePolicy {
	bool valid = false;
	uint32_t minimum = 0;
};

// No row/table preserves ordinary per-character admission. An enabled average
// policy requires min_level=0, so the two different level models cannot mix.
inline AveragePolicy ReadAveragePolicy(Database &db, uint32_t task_id, uint32_t min_level)
{
	auto exists = db.QueryDatabase(
		"SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() "
		"AND TABLE_NAME='shared_task_average_level_policies'");
	if (!exists.Success() || exists.RowCount() != 1) {
		return {};
	}
	const auto tables = Number(exists.begin()[0]);
	if (!tables || *tables > 1) {
		return {};
	}
	if (*tables == 0) {
		return {true, 0};
	}
	auto rows = db.QueryDatabase(fmt::format(
		"SELECT minimum_average_level,enabled FROM shared_task_average_level_policies "
		"WHERE task_id={} LIMIT 2", task_id));
	if (!rows.Success() || rows.RowCount() > 1) {
		return {};
	}
	if (rows.RowCount() == 0) {
		return {true, 0};
	}
	auto row = rows.begin();
	const auto enabled = Number(row[1]);
	if (!enabled || *enabled > 1) {
		return {};
	}
	if (*enabled == 0) {
		return {true, 0};
	}
	const auto minimum = Number(row[0]);
	if (!minimum || *minimum < 1 || *minimum > 255 || min_level != 0) {
		return {};
	}
	return {true, static_cast<uint32_t>(*minimum)};
}

inline bool MeetsAverage(uint64_t total, uint64_t count, uint32_t minimum)
{
	return count != 0 && minimum >= 1 && minimum <= 255 &&
		count <= UINT64_MAX / 255 && total >= count && total <= count * 255 &&
		total >= count * minimum;
}

// Fresh database levels cover offline party members and late invitations.
// Missing, duplicate, invalid or unreadable member rows cannot establish an
// average. No rounding can admit a party whose actual mean is below minimum.
inline std::optional<bool> CheckAverage(
	Database &db, uint32_t minimum, const std::vector<uint32_t> &ids)
{
	if (ids.empty() || std::set<uint32_t>(ids.begin(), ids.end()).size() != ids.size() ||
		std::find(ids.begin(), ids.end(), 0) != ids.end()) {
		return std::nullopt;
	}
	auto rows = db.QueryDatabase(fmt::format(
		"SELECT COUNT(*),COUNT(level),SUM(level),MIN(level),MAX(level) FROM character_data "
		"WHERE id IN ({})", Strings::Join(ids, ",")));
	if (!rows.Success() || rows.RowCount() != 1) {
		return std::nullopt;
	}
	auto row = rows.begin();
	const auto count = Number(row[0]);
	const auto known = Number(row[1]);
	const auto total = Number(row[2]);
	const auto low = Number(row[3]);
	const auto high = Number(row[4]);
	if (!count || !known || !total || !low || !high || *count != ids.size() ||
		*known != *count || *low < 1 || *high > 255) {
		return std::nullopt;
	}
	return MeetsAverage(*total, *count, minimum);
}

} // namespace SharedTaskAdmission
