#pragma once

#include "shared_task_admission.h"
#include "strings.h"

#include <array>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

namespace HouseOfThuleGroup {

struct Mission {
	uint32_t task;
	uint32_t requester_zone;
	uint32_t requester_type;
	uint32_t requester_spawn;
	uint32_t destination_zone;
	uint32_t dz_template;
};

constexpr uint32_t FirstTask = 920380;
constexpr uint32_t InstanceVersion = 20;
constexpr uint32_t FirstAuthoredPrerequisite = 920406;
constexpr std::array Missions{
	Mission{920380, 700, 700075, 138551, 700, 920390},
	Mission{920381, 701, 701022, 140520, 701, 920391},
	Mission{920382, 703, 703044, 139057, 703, 920392},
	Mission{920383, 705, 705004, 137673, 705, 920393},
	Mission{920384, 704, 704000, 140948, 704, 920394},
	Mission{920385, 706, 706047, 138125, 706, 920395},
	Mission{920386, 702, 702000, 140672, 702, 920396},
	Mission{920387, 702, 702051, 140828, 709, 920397},
	Mission{920388, 710, 710047, 139372, 710, 920398},
	Mission{920389, 711, 711084, 140303, 711, 920399}
};

struct Prerequisite {
	uint32_t zone;
	uint32_t npc;
	uint32_t spawn;
	std::array<uint32_t, 4> required{};
};

// These contracts govern the project-authored public prerequisites. The local
// task IDs are contiguous, while the public requester can differ from the mission giver.
constexpr std::array AuthoredPrerequisites{
	Prerequisite{703, 703038, 139032},                                        // 920406
	Prerequisite{703, 703038, 139032, {920406}},
	Prerequisite{703, 703038, 139032, {920406, 920407}},
	Prerequisite{705, 705003, 137672},
	Prerequisite{705, 705003, 137672, {920409}},
	Prerequisite{705, 705003, 137672, {920409, 920410}},
	Prerequisite{704, 704000, 140948},
	Prerequisite{704, 704000, 140948, {920412}},
	Prerequisite{704, 704000, 140948, {920412, 920413}},                       // Book of Secrets
	Prerequisite{706, 706048, 138127, {920432, 920433, 920434, 920435}},
	Prerequisite{702, 702012, 140687},
	Prerequisite{702, 702012, 140687, {920416}},
	Prerequisite{702, 702012, 140687, {920416, 920417}},
	Prerequisite{702, 702012, 140687, {920416, 920417, 920418}},
	Prerequisite{702, 702012, 140687, {920416, 920417, 920418, 920419}},
	Prerequisite{709, 709002, 137743},
	Prerequisite{709, 709036, 137826},
	Prerequisite{709, 709076, 137926},
	Prerequisite{710, 710047, 139372},                                        // Authored sword lead
	Prerequisite{710, 710017, 139279},
	Prerequisite{710, 710068, 139435},                                        // Please Release Me
	Prerequisite{710, 710035, 139313},                                        // Such a Prankster
	Prerequisite{710, 710071, 139441},
	Prerequisite{711, 711012, 140117},
	Prerequisite{711, 711009, 140113, {920429}},
	Prerequisite{711, 711009, 140113, {920429, 920430}},
	Prerequisite{706, 706010, 138054},
	Prerequisite{706, 706010, 138054, {920432}},
	Prerequisite{706, 706047, 138125, {920432, 920433}},
	Prerequisite{706, 706047, 138125, {920432, 920433, 920434}}                  // 920435
};
static_assert(AuthoredPrerequisites.size() == 30);

constexpr const Prerequisite *FindAuthoredPrerequisite(uint32_t task)
{
	if (task < FirstAuthoredPrerequisite || task - FirstAuthoredPrerequisite >= AuthoredPrerequisites.size()) {
		return nullptr;
	}
	return &AuthoredPrerequisites[task - FirstAuthoredPrerequisite];
}

constexpr bool IsSharedPrerequisite(uint32_t id)
{
	return id == 920414 || id == 920426 || id == 920427;
}

inline bool IsPublicGroupLeader(Database &db, uint32_t character_id)
{
	auto leaders = db.QueryDatabase(fmt::format(
		"SELECT 1 FROM group_id g JOIN group_leaders l ON l.gid = g.group_id "
		"JOIN character_data c ON c.id = g.character_id AND c.name = l.leadername "
		"WHERE g.character_id = {} AND g.group_id > 0 LIMIT 2",
		character_id), false);
	return leaders.Success() && leaders.RowCount() == 1;
}

constexpr bool IsPrerequisiteRequester(
	const Prerequisite &prerequisite, uint32_t zone, uint32_t version, uint32_t instance,
	uint32_t npc_type, uint32_t spawn
)
{
	return zone == prerequisite.zone && version == 0 && instance == 0 &&
		npc_type == prerequisite.npc && spawn == prerequisite.spawn;
}

struct TerminalChest {
	uint32_t npc;
	uint32_t zone;
	uint32_t task;
	uint32_t activity;
	uint32_t spawn;
};
constexpr std::array TerminalChests{
	TerminalChest{304101837, 700, 920380, 6, 304101844},
	TerminalChest{304101904, 701, 920381, 7, 304101911},
	TerminalChest{304101949, 703, 920382, 7, 304101957},
	TerminalChest{304102021, 705, 920383, 6, 304102058},
	TerminalChest{304102026, 704, 920384, 6, 304102071},
	TerminalChest{304102031, 706, 920385, 7, 304102082},
	TerminalChest{304102036, 702, 920386, 7, 304102095},
	TerminalChest{304102041, 709, 920387, 6, 304102107},
	TerminalChest{304102046, 710, 920388, 7, 304102119},
	TerminalChest{304102051, 711, 920389, 7, 304102132}
};

constexpr const TerminalChest *FindChest(uint32_t npc)
{
	if (npc >= TerminalChests.front().npc) {
		for (const auto &chest : TerminalChests) {
			if (chest.npc == npc) {
				return &chest;
			}
		}
	}
	return nullptr;
}

// The leader earns these routes before requesting. These are local task IDs,
// including the existing first Rupkerr task and the three shared prerequisites.
constexpr uint32_t RequesterTasks[][5] = {
	{920131, 920400, 920401},
	{920402, 920403, 920404, 920405},
	{920406, 920407, 920408},
	{920409, 920410, 920411},
	{920412, 920413, 920414},
	{920415},
	{920416, 920417, 920418, 920419, 920420},
	{920421, 920422, 920423},
	{920424, 920425, 920426, 920427, 920428},
	{920429, 920430, 920431}
};
static_assert(std::size(RequesterTasks) == Missions.size());

constexpr const Mission *Find(uint32_t task)
{
	if (task < FirstTask || task - FirstTask >= Missions.size()) {
		return nullptr;
	}
	return &Missions[task - FirstTask];
}

inline bool IsRequester(
	const Mission &mission, uint32_t zone, uint32_t version, uint32_t instance,
	uint32_t npc_type, uint32_t spawn
)
{
	return zone == mission.requester_zone && version == 0 && instance == 0 &&
		npc_type == mission.requester_type && spawn == mission.requester_spawn;
}

inline bool HasEarnedTasks(Database &db, uint32_t character_id, std::span<const uint32_t> required)
{
	if (!character_id) {
		return false;
	}

	std::vector<uint32_t> personal;
	std::vector<uint32_t> shared;
	for (const auto id : required) {
		if (IsSharedPrerequisite(id) || Find(id)) {
			shared.push_back(id);
		} else if (id) {
			personal.push_back(id);
		}
	}

	if (!personal.empty()) {
		auto personal_completion = db.QueryDatabase(fmt::format(
			"SELECT COUNT(DISTINCT taskid) FROM completed_tasks WHERE charid={} "
			"AND activityid=-1 AND completedtime>0 AND taskid IN ({})",
			character_id, Strings::Join(personal, ",")), false);
		if (!personal_completion.Success() || personal_completion.RowCount() != 1 ||
			SharedTaskAdmission::Number(personal_completion.begin()[0]) != personal.size()) {
			return false;
		}
	}
	if (shared.empty()) {
		return true;
	}

	auto shared_completion = db.QueryDatabase(fmt::format(
		"SELECT COUNT(DISTINCT t.task_id) FROM completed_shared_tasks t "
		"JOIN completed_shared_task_members m ON m.shared_task_id=t.id "
		"WHERE m.character_id={} AND t.completion_time>=t.accepted_time AND t.task_id IN ({})",
		character_id, Strings::Join(shared, ",")), false);
	return shared_completion.Success() && shared_completion.RowCount() == 1 &&
		SharedTaskAdmission::Number(shared_completion.begin()[0]) == shared.size();
}

inline bool HasEarnedRequest(Database &db, const Mission &mission, uint32_t character_id)
{
	const auto &requester = RequesterTasks[mission.task - FirstTask];
	std::vector<uint32_t> required(std::begin(requester), std::end(requester));
	// The opening missions lead to Tier 2, and the first six to Tier 3.
	// The final Tier 3 requester also needs the preceding three missions.
	if (mission.task >= 920382) {
		const uint32_t end = mission.task == 920389 ? 920389 :
			(mission.task >= 920386 ? 920386 : 920382);
		for (uint32_t id = FirstTask; id < end; ++id) {
			required.push_back(id);
		}
	}
	return HasEarnedTasks(db, character_id, required);
}

} // namespace HouseOfThuleGroup
