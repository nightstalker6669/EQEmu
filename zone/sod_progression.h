#pragma once

#include "common/database.h"
#include "common/rulesys.h"
#include "common/shared_task_admission.h"
#include "common/sod_progression.h"
#include "common/sod_task_activation.h"
#include "common/strings.h"

#include <map>
#include <string>
#include <vector>

namespace SodProgression {

struct TaskRead {
	bool available = false;
	std::map<uint32_t, Credit> completion;
	std::map<uint32_t, std::string> titles;
};

// A fresh read of committed native history is the acknowledgement boundary.
// The personal -1 record marks native completion. Shared history is published
// by SharedTaskCompletion::Archive in a strict transaction with its members.
// No new player flag/cache is written, and a failed read cannot grant credit.
inline TaskRead ReadCommittedTasks(Database& db, Database& content, uint32_t character, const std::vector<uint32_t>& tasks)
{
	TaskRead out;
	if (!character || tasks.empty()) return out;
	for (const auto id : tasks) {
		if (!IsReservedOceangreenTask(id)) return out;
	}
	// Content and character databases may be separate schemas/servers. Never
	// join tasks to player history through an assumed single DB connection.
	auto definitions = content.QueryDatabase(fmt::format(
		"SELECT id,title FROM tasks WHERE id IN ({}) ORDER BY id", Strings::Join(tasks, ",")));
	if (!definitions.Success() || definitions.RowCount() != tasks.size()) return out;
	for (auto row : definitions) {
		const auto id = SharedTaskAdmission::Number(row[0]);
		if (!id || *id > UINT32_MAX || !row[1] || out.titles.count(static_cast<uint32_t>(*id))) return TaskRead{};
		out.titles.emplace(static_cast<uint32_t>(*id), row[1]);
	}
	std::vector<std::string> requested;
	for (const auto id : tasks) requested.push_back(fmt::format("SELECT {} AS taskid", id));
	auto result = db.QueryDatabase(fmt::format(
		"SELECT t.taskid,"
		"EXISTS(SELECT 1 FROM completed_tasks ct WHERE ct.charid={} "
		"AND ct.taskid=t.taskid AND ct.activityid=-1 AND ct.completedtime>0),"
		"EXISTS(SELECT 1 FROM completed_shared_tasks st "
		"JOIN completed_shared_task_members sm ON sm.shared_task_id=st.id "
		"WHERE sm.character_id={} AND st.task_id=t.taskid "
		"AND st.completion_time IS NOT NULL AND UNIX_TIMESTAMP(st.completion_time)>0) "
		"FROM ({}) t ORDER BY t.taskid", character, character, Strings::Join(requested, " UNION ALL ")));
	if (!result.Success() || result.RowCount() != tasks.size()) return TaskRead{};
	for (auto row : result) {
		const auto id = SharedTaskAdmission::Number(row[0]);
		const auto personal = SharedTaskAdmission::Number(row[1]);
		const auto shared = SharedTaskAdmission::Number(row[2]);
		if (!id || *id > UINT32_MAX || !out.titles.count(static_cast<uint32_t>(*id)) ||
			!personal || !shared || *personal > 1 || *shared > 1 ||
			out.completion.count(static_cast<uint32_t>(*id))) return TaskRead{};
		out.completion.emplace(static_cast<uint32_t>(*id),
			*personal || *shared ? Credit::Complete : Credit::Incomplete);
	}
	out.available = true;
	return out;
}

inline Evidence ReadOceangreenSlice(Database& db, Database& content, uint32_t character)
{
	Evidence evidence;
	if (!RuleB(SodProgression, EnableOceangreenSlice)) return evidence;
	std::vector<uint32_t> ids;
	for (const auto& t : OceangreenGroupTasks) ids.push_back(t.id);
	const auto tasks = ReadCommittedTasks(db, content, character, ids);
	if (!tasks.available) return evidence;
	Credit complete = Credit::Complete;
	for (const auto& t : OceangreenGroupTasks) {
		const auto title = tasks.titles.find(t.id);
		const auto credit = tasks.completion.find(t.id);
		if (title == tasks.titles.end() || title->second != t.title || credit == tasks.completion.end()) return Evidence{};
		if (credit->second != Credit::Complete) complete = Credit::Incomplete;
	}
	evidence.available = true;
	evidence.group[static_cast<unsigned>(Theme::Oceangreen)] = complete;
	return evidence;
}

// A base Blackburrow visit is allowed during Summary Execution, then forever
// from its acknowledged native completion. No invented key or player flag.
inline bool BlackburrowAccess(Database& db, Database& content, uint32_t character, bool native_active)
{
	const auto history = ReadCommittedTasks(db, content, character, {920181});
	if (!history.available) return false;
	if (history.completion.at(920181) == Credit::Complete) return true;
	if (!native_active) return false;
	auto active = db.QueryDatabase(fmt::format(
		"SELECT EXISTS(SELECT 1 FROM character_tasks WHERE charid={} AND taskid=920181 "
		"AND type=2 AND acceptedtime>0 AND was_rewarded=0)", character));
	if (!active.Success() || active.RowCount()!=1) return false;
	const auto found = SharedTaskAdmission::Number(active.begin()[0]);
	return found && *found==1;
}

inline uint32_t OceangreenMissionZone(uint32_t task)
{
	switch (task) {
	case 920178: case 920179: return 467;
	case 920180: case 920183: return 468;
	case 920185: case 920186: case 920187: return 469;
	default: return 0; // original raid identities are not bound
	}
}

// Verify the actual native shared occurrence, roster, DZ association and
// instance zone/version. No requester prerequisites are imposed on members.
// Version numbers are LOCAL content mappings from reviewed templates, not
// guessed historical Live version IDs. A missing mapping always refuses.
inline bool MissionEntry(Database& db, Database& content, uint32_t character,
	uint64_t occurrence, uint32_t destination, int version, uint32_t instance, bool preflight)
{
	if (!character || !occurrence || version<=0 || (!instance && !preflight)) return false;
	auto native = db.QueryDatabase(fmt::format(
		"SELECT st.task_id FROM shared_tasks st "
		"JOIN shared_task_members sm ON sm.shared_task_id=st.id AND sm.character_id={} "
		"JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=st.id "
		"JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4 "
		"JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=sm.character_id "
		"JOIN instance_list i ON i.id=dz.instance_id "
		"WHERE st.id={} AND i.zone={} AND i.version={} AND dz.instance_id>0 {}",
		character, occurrence, destination, version,
		instance ? fmt::format("AND i.id={}",instance) : ""));
	if (!native.Success() || native.RowCount()!=1) return false;
	const auto task = SharedTaskAdmission::Number(native.begin()[0]);
	if (!task || *task>UINT32_MAX || OceangreenMissionZone(static_cast<uint32_t>(*task))!=destination) return false;
	auto mapping = content.QueryDatabase(fmt::format(
		"SELECT t.type,t.dz_template_id,p.zone_id,p.zone_version FROM tasks t "
		"JOIN dynamic_zone_templates p ON p.id=t.dz_template_id WHERE t.id={}",*task));
	if (!mapping.Success() || mapping.RowCount()!=1) return false;
	auto row = mapping.begin();
	const auto type=SharedTaskAdmission::Number(row[0]), templ=SharedTaskAdmission::Number(row[1]);
	const auto zone=SharedTaskAdmission::Number(row[2]), ver=SharedTaskAdmission::Number(row[3]);
	// tasks.enabled governs NEW requests/joins. Existing admitted members are
	// not evicted when a definition is disabled; native expiry still applies.
	return type && *type==1 && templ && *templ==*task && zone && *zone==destination &&
		ver && *ver==static_cast<uint32_t>(version);
}

inline bool AllowsOceangreenSliceBase(const Evidence& evidence, uint32_t destination)
{
	if (!evidence.available) return false;
	// Original Temple base is unkeyed, reached through Village's stump.
	if (destination==466 || destination==467 || destination==469) return true;
	return destination==459 || (destination==460 &&
		evidence.group[static_cast<unsigned>(Theme::Oceangreen)]==Credit::Complete);
}

} // namespace SodProgression
