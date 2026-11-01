#pragma once

#include "common/database.h"
#include "common/strings.h"

#include <charconv>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace SharedTaskAdmission {

inline std::optional<uint64_t> Number(const char *value)
{
	if (!value || !*value) {
		return std::nullopt;
	}
	const std::string_view text(value);
	uint64_t number = 0;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), number);
	if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
		return std::nullopt;
	}
	return number;
}

struct Policy {
	bool valid = false;
	uint32_t late_join_min_level = 0;
};

// An absent optional content table is the legacy policy. A failed query or
// malformed enabled override is not an absent policy and must fail closed.
inline Policy ReadPolicy(Database &db, uint32_t task_id, uint32_t min_level, uint32_t max_level)
{
	auto exists = db.QueryDatabase(
		"SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA = DATABASE() "
		"AND TABLE_NAME = 'shared_task_admission_policies'");
	if (!exists.Success() || exists.RowCount() != 1) {
		return {};
	}
	const auto count = Number(exists.begin()[0]);
	if (!count || *count > 1) {
		return {};
	}
	if (!*count) {
		return {true, min_level};
	}
	auto result = db.QueryDatabase(fmt::format(
		"SELECT late_join_min_level, enabled FROM shared_task_admission_policies "
		"WHERE task_id = {} LIMIT 2", task_id));
	if (!result.Success() || result.RowCount() > 1) {
		return {};
	}
	if (!result.RowCount()) {
		return {true, min_level};
	}
	auto row = result.begin();
	const auto enabled = Number(row[1]);
	if (!enabled || *enabled > 1) {
		return {};
	}
	if (!*enabled) {
		return {true, min_level};
	}
	const auto level = Number(row[0]);
	if (!level || !*level || *level > 255 || !min_level || *level > min_level ||
		(max_level && *level > max_level)) {
		return {};
	}
	return {true, static_cast<uint32_t>(*level)};
}

// nullopt is an unreadable/inconsistent history; zero means no completion.
// The archive join is authoritative even when zone completion caches are not
// populated. An orphan member row cannot safely establish which task ran.
inline std::optional<uint32_t> CompletedMember(
	Database &db, uint32_t task_id, const std::vector<uint32_t> &character_ids)
{
	const std::set<uint32_t> ids(character_ids.begin(), character_ids.end());
	if (!task_id || ids.empty() || ids.count(0) || ids.size() != character_ids.size()) {
		return std::nullopt;
	}
	auto result = db.QueryDatabase(fmt::format(
		"SELECT m.character_id, t.id, t.task_id FROM completed_shared_task_members m "
		"LEFT JOIN completed_shared_tasks t ON t.id = m.shared_task_id "
		"WHERE m.character_id IN ({}) AND (t.task_id = {} OR t.id IS NULL OR t.task_id IS NULL OR t.task_id <= 0) "
		"ORDER BY (t.id IS NULL) DESC LIMIT 1", Strings::Join(character_ids, ","), task_id));
	if (!result.Success() || result.RowCount() > 1) {
		return std::nullopt;
	}
	if (!result.RowCount()) {
		return 0;
	}
	auto row = result.begin();
	const auto member = Number(row[0]);
	const auto run = Number(row[1]);
	const auto archived_task = Number(row[2]);
	if (!member || *member > UINT32_MAX || !archived_task || *archived_task != task_id || !ids.count(static_cast<uint32_t>(*member)) || !run || !*run) {
		return std::nullopt;
	}
	return static_cast<uint32_t>(*member);
}

} // namespace SharedTaskAdmission
