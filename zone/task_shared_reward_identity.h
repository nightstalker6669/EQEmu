#ifndef EQEMU_TASK_SHARED_REWARD_IDENTITY_H
#define EQEMU_TASK_SHARED_REWARD_IDENTITY_H

#include "../common/database.h"
#include <cstdlib>
#include <cstdint>
#include <optional>

namespace TaskSharedRewardIdentity {

// Each shared run is identified by world's uint64 primary key. A timestamp
// alone cannot distinguish same-second repeat requests or stale local state.
struct Run {
	uint64_t id = 0;
	uint32_t task_id = 0;
	uint32_t accepted_time = 0;
};

inline uint64_t Number(const char *value)
{
	return value ? std::strtoull(value, nullptr, 10) : 0;
}

inline std::optional<Run> ReadRun(Database &db, uint32_t character_id, uint32_t task_id)
{
	auto result = db.QueryDatabase(fmt::format(
		"SELECT s.id, s.task_id, UNIX_TIMESTAMP(s.accepted_time) FROM shared_task_members m "
		"JOIN shared_tasks s ON s.id = m.shared_task_id "
		"WHERE m.character_id = {} AND s.task_id = {}",
		character_id, task_id
	));
	if (!result.Success() || result.RowCount() != 1) {
		return std::nullopt;
	}
	auto row = result.begin();
	Run run{Number(row[0]), static_cast<uint32_t>(Number(row[1])),
		static_cast<uint32_t>(Number(row[2]))};
	return run.id && run.task_id ? std::optional<Run>(run) : std::nullopt;
}

// nullopt is a DB failure: callers must not discard state or grant a reward.
// No world membership is handled by the existing shared-task removal path.
inline std::optional<bool> LocalMatches(
	Database &db, uint32_t character_id, uint32_t task_id, uint32_t accepted_time)
{
	auto result = db.QueryDatabase(fmt::format(
		"SELECT s.task_id, UNIX_TIMESTAMP(s.accepted_time), h.occurrence_id, r.occurrence_id "
		"FROM shared_task_members m JOIN shared_tasks s ON s.id = m.shared_task_id "
		"LEFT JOIN character_shared_task_reward_instances h ON "
		"h.character_id = m.character_id AND h.shared_task_id = s.id "
		"LEFT JOIN character_task_reward_instances r ON "
		"r.character_id = m.character_id AND r.task_id = s.task_id "
		"WHERE m.character_id = {}", character_id
	));
	if (!result.Success() || result.RowCount() > 1) {
		return std::nullopt;
	}
	if (result.RowCount() == 0) {
		return true;
	}
	auto row = result.begin();
	return Number(row[0]) == task_id && Number(row[1]) == accepted_time &&
		Number(row[2]) != 0 && Number(row[2]) == Number(row[3]);
}

inline std::optional<uint64_t> Ensure(
	Database &db, uint32_t character_id, uint32_t task_id, uint32_t accepted_time)
{
	const auto run = ReadRun(db, character_id, task_id);
	if (!run || run->accepted_time != accepted_time) {
		return std::nullopt;
	}
	const auto read_mapping = [&]() {
		return db.QueryDatabase(fmt::format(
			"SELECT task_id, occurrence_id FROM character_shared_task_reward_instances "
			"WHERE character_id = {} AND shared_task_id = {}",
			character_id, run->id
		));
	};
	auto mapping = read_mapping();
	if (!mapping.Success() || mapping.RowCount() > 1) {
		return std::nullopt;
	}
	if (mapping.RowCount() == 0) {
		// Use the existing global allocator so shared and solo source instance
		// identities cannot collide. An interrupted unused allocation is safe.
		auto reserved = db.QueryDatabase(fmt::format(
			"REPLACE INTO character_task_reward_instances "
			"(character_id, task_id, accepted_time) VALUES ({}, {}, {})",
			character_id, task_id, accepted_time
		));
		if (!reserved.Success() || !reserved.LastInsertedID()) {
			return std::nullopt;
		}
		auto inserted = db.QueryDatabase(fmt::format(
			"INSERT IGNORE INTO character_shared_task_reward_instances "
			"(character_id, shared_task_id, task_id, occurrence_id) "
			"VALUES ({}, {}, {}, {})",
			character_id, run->id, task_id, reserved.LastInsertedID()
		));
		if (!inserted.Success()) {
			return std::nullopt;
		}
		mapping = read_mapping();
	}
	if (!mapping.Success() || mapping.RowCount() != 1) {
		return std::nullopt;
	}
	auto row = mapping.begin();
	const auto occurrence_id = Number(row[1]);
	if (Number(row[0]) != task_id || !occurrence_id) {
		return std::nullopt;
	}
	// The append-only run mapping survives cancellation. Restore only the
	// active pointer; leave all previous selections and grant ledgers intact.
	auto pointer = db.QueryDatabase(fmt::format(
		"REPLACE INTO character_task_reward_instances "
		"(occurrence_id, character_id, task_id, accepted_time) "
		"VALUES ({}, {}, {}, {})",
		occurrence_id, character_id, task_id, accepted_time
	));
	return pointer.Success() ? std::optional<uint64_t>(occurrence_id) : std::nullopt;
}

} // namespace TaskSharedRewardIdentity
#endif
