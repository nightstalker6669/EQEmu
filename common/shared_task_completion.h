#pragma once

#include "common/database.h"
#include "common/shared_task_admission.h"

#include <optional>
#include <set>
#include <vector>

namespace SharedTaskCompletion {

struct Header {
	uint64_t id;
	uint32_t task_id;
	int64_t accepted_time;
	int64_t expire_time;
	int64_t completion_time;
};

struct Member {
	uint32_t character_id;
	bool is_leader;
};

struct Activity {
	uint32_t activity_id;
	uint32_t done_count;
	int64_t updated_time;
	int64_t completed_time;
};

// Uses the existing strict transaction (no reconnect/retry inside a failed
// transaction). No active row is removed, and no reward mapping is modified.
inline std::optional<int64_t> Archive(
	Database &db, const Header &task, const std::vector<Member> &members,
	const std::vector<Activity> &activities)
{
	if (!task.id || !task.task_id || task.accepted_time <= 0 ||
		task.completion_time <= 0 || activities.empty()) {
		return std::nullopt;
	}
	std::set<uint32_t> ids;
	for (const auto &member : members) {
		if (!member.character_id || !ids.insert(member.character_id).second) {
			return std::nullopt;
		}
	}
	ids.clear();
	for (const auto &activity : activities) {
		if (!ids.insert(activity.activity_id).second) {
			return std::nullopt;
		}
	}
	if (!db.TransactionBeginStrict().Success()) {
		return std::nullopt;
	}
	struct Rollback {
		Database &db;
		~Rollback() { db.TransactionRollbackStrict(); }
	} rollback{db};

	auto active = db.QueryDatabase(fmt::format(
		"SELECT id FROM shared_tasks WHERE id = {} AND task_id = {} "
		"AND accepted_time <=> FROM_UNIXTIME({}) FOR UPDATE",
		task.id, task.task_id, task.accepted_time));
	if (!active.Success() || active.RowCount() != 1) {
		return std::nullopt;
	}
	const auto date = [](int64_t time) {
		return time > 0 ? fmt::format("FROM_UNIXTIME({})", time) : std::string("NULL");
	};
	auto header = db.QueryDatabase(fmt::format(
		"INSERT INTO completed_shared_tasks "
		"(id, task_id, accepted_time, expire_time, completion_time, is_locked) "
		"VALUES ({}, {}, {}, {}, {}, 1) ON DUPLICATE KEY UPDATE id = id",
		task.id, task.task_id, date(task.accepted_time), date(task.expire_time), date(task.completion_time)));
	if (!header.Success()) {
		return std::nullopt;
	}
	auto existing = db.QueryDatabase(fmt::format(
		"SELECT task_id, UNIX_TIMESTAMP(accepted_time), UNIX_TIMESTAMP(completion_time), is_locked, "
		"(expire_time <=> {}) FROM completed_shared_tasks WHERE id = {}",
		date(task.expire_time), task.id));
	if (!existing.Success() || existing.RowCount() != 1) {
		return std::nullopt;
	}
	auto row = existing.begin();
	const auto archived_task = SharedTaskAdmission::Number(row[0]);
	const auto accepted = SharedTaskAdmission::Number(row[1]);
	const auto completed = SharedTaskAdmission::Number(row[2]);
	const auto locked = SharedTaskAdmission::Number(row[3]);
	const auto expires_match = SharedTaskAdmission::Number(row[4]);
	if (!archived_task || *archived_task != task.task_id || !accepted ||
		*accepted != static_cast<uint64_t>(task.accepted_time) || !completed || !*completed ||
		*completed > INT64_MAX || !locked || *locked != 1 || !expires_match || *expires_match != 1) {
		return std::nullopt;
	}

	ids.clear();
	// Append missing current members. Never replace/delete historical members.
	std::vector<std::string> values;
	for (const auto &member : members) {
		values.emplace_back(fmt::format("({}, {}, {})", task.id, member.character_id, member.is_leader ? 1 : 0));
		ids.insert(member.character_id);
	}
	if (!values.empty() && !db.QueryDatabase(fmt::format(
		"INSERT INTO completed_shared_task_members (shared_task_id, character_id, is_leader) "
		"VALUES {} ON DUPLICATE KEY UPDATE shared_task_id = shared_task_id", Strings::Join(values, ","))).Success()) {
		return std::nullopt;
	}
	// No current members is safe only if an earlier archive retained members.
	auto member_count = db.QueryDatabase(fmt::format(
		"SELECT COUNT(*) FROM completed_shared_task_members WHERE shared_task_id = {}", task.id));
	if (!member_count.Success() || member_count.RowCount() != 1) {
		return std::nullopt;
	}
	const auto count = SharedTaskAdmission::Number(member_count.begin()[0]);
	if (!count || !*count || *count < members.size()) {
		return std::nullopt;
	}

	if (!members.empty()) {
		std::vector<uint32_t> member_ids(ids.begin(), ids.end());
		auto verified_members = db.QueryDatabase(fmt::format(
			"SELECT COUNT(*) FROM completed_shared_task_members WHERE shared_task_id = {} AND character_id IN ({})",
			task.id, Strings::Join(member_ids, ",")));
		if (!verified_members.Success() || verified_members.RowCount() != 1 ||
			SharedTaskAdmission::Number(verified_members.begin()[0]) != std::optional<uint64_t>(members.size())) {
			return std::nullopt;
		}
	}

	values.clear();
	std::vector<std::string> predicates;
	for (const auto &activity : activities) {
		values.emplace_back(fmt::format("({}, {}, {}, {}, {})", task.id, activity.activity_id,
			activity.done_count, date(activity.updated_time), date(activity.completed_time)));
		predicates.emplace_back(fmt::format(
			"(activity_id = {} AND done_count = {} AND updated_time <=> {} AND completed_time <=> {})",
			activity.activity_id, activity.done_count, date(activity.updated_time), date(activity.completed_time)));
	}
	if (!db.QueryDatabase(fmt::format(
		"INSERT INTO completed_shared_task_activity_state "
		"(shared_task_id, activity_id, done_count, updated_time, completed_time) "
		"VALUES {} ON DUPLICATE KEY UPDATE shared_task_id = shared_task_id", Strings::Join(values, ","))).Success()) {
		return std::nullopt;
	}
	auto verified = db.QueryDatabase(fmt::format(
		"SELECT COUNT(*) FROM completed_shared_task_activity_state WHERE shared_task_id = {} AND ({})",
		task.id, Strings::Join(predicates, " OR ")));
	if (!verified.Success() || verified.RowCount() != 1 ||
		SharedTaskAdmission::Number(verified.begin()[0]) != std::optional<uint64_t>(activities.size())) {
		return std::nullopt;
	}

	// Persist the final active snapshot in the same commit as the archive.
	if (!db.QueryDatabase(fmt::format(
		"DELETE FROM shared_task_activity_state WHERE shared_task_id = {}", task.id)).Success() ||
		!db.QueryDatabase(fmt::format(
			"INSERT INTO shared_task_activity_state "
			"(shared_task_id, activity_id, done_count, updated_time, completed_time) VALUES {}",
			Strings::Join(values, ","))).Success() ||
		!db.QueryDatabase(fmt::format(
			"UPDATE shared_tasks SET completion_time = FROM_UNIXTIME({}), is_locked = 1 WHERE id = {}",
			*completed, task.id)).Success() || !db.TransactionCommitStrict().Success()) {
		return std::nullopt;
	}
	return static_cast<int64_t>(*completed);
}

} // namespace SharedTaskCompletion
