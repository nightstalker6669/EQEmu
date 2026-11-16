#pragma once
#include "common/database.h"
#include "common/rulesys.h"
#include "common/shared_task_admission.h"
#include "common/sod_progression.h"

namespace SodProgression {
// Applies to NEW requests/admissions, never an earned completion or existing
// membership reload. Both World and Zone consult fresh content definitions.
inline bool MayStartOrJoinTask(Database& content, uint32_t task, bool shared_only = false)
{
	if (IsKithicorTask(task)) {
		if (!RuleB(SodProgression, EnableKithicorSlice) || !RuleB(TaskSystem, RecordCompletedTasks)) return false;
		const bool shared = IsKithicorSharedTask(task);
		if (shared_only && !shared) return false;
		auto result = content.QueryDatabase(fmt::format("SELECT enabled,type FROM tasks WHERE id={}", task));
		if (!result.Success() || result.RowCount() != 1) return false;
		auto row = result.begin();
		const auto enabled = SharedTaskAdmission::Number(row[0]);
		const auto type = SharedTaskAdmission::Number(row[1]);
		return enabled && *enabled == 1 && type && *type == (shared ? 1u : 2u);
	}
	if (!IsOceangreenRequest(task)) return true;
	if((task>=920184&&task<=920189)||!RuleB(TaskSystem,RecordCompletedTasks))return false; // Outside approved first slice.
	if (!RuleB(SodProgression, EnableOceangreenSlice) || !IsReservedOceangreenTask(task)) return false;
	const bool shared = task==920178 || task==920179 || task==920180 ||
		task==920183 || task==920185 || task==920186 || task==920187;
	if (shared_only && !shared) return false;
	auto result = content.QueryDatabase(fmt::format("SELECT enabled,type FROM tasks WHERE id={}", task));
	if (!result.Success() || result.RowCount() != 1) return false;
	auto row = result.begin();
	const auto enabled = SharedTaskAdmission::Number(row[0]);
	const auto type = SharedTaskAdmission::Number(row[1]);
	return enabled && *enabled==1 && type && *type==(shared ? 1u : 2u);
}
} // namespace SodProgression
