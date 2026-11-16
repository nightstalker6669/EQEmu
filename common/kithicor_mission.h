#pragma once

#include "common/database.h"
#include "common/shared_task_admission.h"
#include "common/sod_progression.h"
#include <array>
#include <optional>

namespace KithicorMission {
inline constexpr uint32_t Model = 1;
struct Phase {
	uint32_t task, phase, template_id, zone, version;
	int required_activity;
};
// Database namespace amendment002, not original Live template/version IDs.
inline constexpr std::array<Phase, 3> Phases{{
	{920192, 0, 920191, 457, 1, -1},
	{920192, 1, 920192, 456, 1, 5},
	{920193, 0, 920193, 457, 2, -1}
}};
inline const Phase* Find(uint32_t task, uint32_t zone, uint32_t version)
{
	for (const auto& p : Phases) if (p.task==task && p.zone==zone && p.version==version) return &p;
	return nullptr;
}
inline bool Ready(Database& db, uint64_t run, const Phase& phase, uint32_t dz, uint32_t instance)
{
	if (!run || !dz || !instance) return false;
	auto r=db.QueryDatabase(fmt::format(
		"SELECT dynamic_zone_id,instance_id FROM kithicor_mission_phases WHERE shared_task_id={} "
		"AND phase={} AND task_id={} AND model_version={} AND ready=1 AND scene_state<>3",
		run,phase.phase,phase.task,Model));
	if (!r.Success() || r.RowCount()!=1) return false;
	auto row=r.begin();
	const auto d=SharedTaskAdmission::Number(row[0]), i=SharedTaskAdmission::Number(row[1]);
	return d && i && *d==dz && *i==instance;
}
inline bool PriorSuccess(Database& db, uint64_t run, const Phase& phase)
{
	if (phase.phase==0) return true; // Explicit sentinel, not a guessed activity0 gate.
	auto r=db.QueryDatabase(fmt::format(
		"SELECT scene_state FROM kithicor_mission_phases WHERE shared_task_id={} AND phase={} "
		"AND task_id={} AND model_version={} AND ready=1",run,phase.phase-1,phase.task,Model));
	if (!r.Success() || r.RowCount()!=1) return false;
	const auto state=SharedTaskAdmission::Number(r.begin()[0]);
	if (!state || *state!=2) return false;
	auto hail=db.QueryDatabase(fmt::format(
		"SELECT done_count FROM shared_task_activity_state WHERE shared_task_id={} AND activity_id={} "
		"AND completed_time IS NOT NULL AND UNIX_TIMESTAMP(completed_time)>0",run,phase.required_activity));
	if (!hail.Success() || hail.RowCount()!=1) return false;
	const auto count=SharedTaskAdmission::Number(hail.begin()[0]);
	return count && *count>=1;
}
} // namespace KithicorMission
