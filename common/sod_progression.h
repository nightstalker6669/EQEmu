#pragma once

#include <array>
#include <cstdint>

// Original SoD entitlement arithmetic. Inputs must come from acknowledged
// native completion evidence, never acceptance, a hail or in-memory progress.
namespace SodProgression {

enum class Credit : int8_t { Unknown = -1, Incomplete = 0, Complete = 1 };
enum class Theme : uint8_t { Oceangreen, Kithicor, FieldOfScale, Rathe };
enum class VoidStage : uint8_t { A, B, C, D, E, F, G };

struct Evidence {
	std::array<Credit, 4> group{{Credit::Unknown, Credit::Unknown, Credit::Unknown, Credit::Unknown}};
	std::array<Credit, 4> raid{{Credit::Unknown, Credit::Unknown, Credit::Unknown, Credit::Unknown}};
	Credit kuua_and_korafax = Credit::Unknown;
	Credit tower = Credit::Unknown;
	bool available = false;
};

constexpr Credit Either(Credit a, Credit b)
{
	return a == Credit::Complete || b == Credit::Complete ? Credit::Complete :
		a == Credit::Incomplete && b == Credit::Incomplete ? Credit::Incomplete : Credit::Unknown;
}

constexpr Credit All(const std::array<Credit, 4>& values)
{
	bool unknown = false;
	for (const auto c : values) {
		if (c == Credit::Incomplete) return Credit::Incomplete;
		unknown = unknown || c == Credit::Unknown;
	}
	return unknown ? Credit::Unknown : Credit::Complete;
}

constexpr Credit Korafax(const Evidence& evidence)
{
	return evidence.available ? Either(All(evidence.group), All(evidence.raid)) : Credit::Unknown;
}

// This is the highest positively proved stage, not a claim that unbound paths
// are incomplete. The Oceangreen and Kithicor group paths are bound;
// Field of Scale is bound by its five group missions when enabled;
// Rathe is bound by all seven original tasks when enabled; raid aggregation and F/G remain unknown.
constexpr VoidStage ProvenVoidStage(const Evidence& evidence)
{
	unsigned count = 0;
	if (!evidence.available) return VoidStage::A;
	for (unsigned i = 0; i < evidence.group.size(); ++i) {
		if (Either(evidence.group[i], evidence.raid[i]) == Credit::Complete) ++count;
	}
	if (count == 4 && Korafax(evidence) == Credit::Complete &&
		evidence.kuua_and_korafax == Credit::Complete) {
		return evidence.tower == Credit::Complete ? VoidStage::G : VoidStage::F;
	}
	return static_cast<VoidStage>(count);
}

constexpr uint32_t VoidZone(VoidStage stage) { return 459 + static_cast<uint32_t>(stage); }
constexpr bool IsVoidZone(uint32_t zone) { return zone >= 459 && zone <= 465; }
constexpr bool IsSodZone(uint32_t zone) { return zone >= 452 && zone <= 477; }

constexpr Credit JourneymanTier(const Evidence& evidence, unsigned tier)
{
	if (!evidence.available || tier < 1 || tier > 5) return Credit::Unknown;
	return tier == 1 ? Credit::Complete : evidence.group[tier - 2];
}

// March22,2009 direct completion report (Alla quest4661) names these FOUR
// completions without the Sergeant tasks. Those two remain requester gates;
// the broader January guide's six-entry route is sufficient, not mandatory.
// These are locally reserved server task IDs, not historical Live task IDs.
struct TaskBinding { uint32_t id; const char* title; };
inline constexpr std::array<TaskBinding, 4> OceangreenGroupTasks{{
	{920177, "Familiarity is Key"},
	{920178, "Stop the Contamination"},
	{920179, "A Dread Challenger"},
	{920180, "Beat the Blackburrow Boss"}
}};

inline constexpr std::array<TaskBinding, 3> KithicorGroupTasks{{
	{920191, "Rain of Ruin"},
	{920192, "Safe Passage"},
	{920193, "Bitter Victuals"}
}};
constexpr bool IsKithicorTask(uint32_t task) { return task >= 920191 && task <= 920196; }
constexpr bool IsKithicorSharedTask(uint32_t task) { return task == 920192 || task == 920193; }

// The original Gaining Audience With Jaled'Dar achievement requires FOUR
// distinct completions from its eight entries (period report, Alla4718,
// August2012). The caller supplies one acknowledged result per bound task,
// never repeat counts or participant-wide requester requirements.
constexpr bool FieldOfScaleAudienceReady(const std::array<Credit, 8>& completed)
{
 unsigned count=0;
 for (const auto credit:completed) {
  if (credit==Credit::Unknown) return false;
  if (credit==Credit::Complete) ++count;
 }
 return count>=4;
}

inline constexpr std::array<TaskBinding, 19> FieldOfScaleTasks{{
	{920197, "Drake Scourge"},
	{920198, "Food Supplies"},
	{920199, "The Grand Illusion"},
	{920200, "Raising Cabilis"},
	{920201, "Sarnak Recovery"},
	{920202, "Sarnak Recovery"},
	{920203, "Sarnak Recovery"},
	{920204, "Supplies"},
	{920205, "Evil Afoot"},
	{920206, "Idle Hands"},
	{920207, "Purify the Body"},
	{920208, "The Skull of the Ancient"},
	{920209, "Lost and Found"},
	{920210, "Parley with Ganak"},
	{920211, "Audience with the Warmaster"},
	{920212, "The Brood of an Emperor"},
	{920213, "The Price of Knowledge"},
	{920214, "Defeat the Dragonbane Phalanx"},
	{920215, "An Impenetrable Shield"}
}};
inline constexpr std::array<TaskBinding, 8> FieldOfScaleAudienceTasks{{
	{920197, "Drake Scourge"},
	{920198, "Food Supplies"},
	{920199, "The Grand Illusion"},
	{920200, "Raising Cabilis"},
	{920201, "Sarnak Recovery"},
	{920202, "Sarnak Recovery"},
	{920203, "Sarnak Recovery"},
	{920204, "Supplies"}
}};
inline constexpr std::array<TaskBinding, 5> FieldOfScaleGroupTasks{{
	{920211, "Audience with the Warmaster"},
	{920212, "The Brood of an Emperor"},
	{920213, "The Price of Knowledge"},
	{920214, "Defeat the Dragonbane Phalanx"},
	{920215, "An Impenetrable Shield"}
}};
constexpr bool IsFieldOfScaleTask(uint32_t id) { for (const auto& t:FieldOfScaleTasks) if (t.id==id) return true; return false; }
constexpr bool IsFieldOfScaleSharedTask(uint32_t id) { return id==920197 || id==920200 || id==920201 || id==920202 || id==920203 || id==920209 || id==920210 || id==920211 || id==920212 || id==920213 || id==920214 || id==920215; }

constexpr uint32_t FieldOfScaleMissionZone(uint32_t id) {
 switch(id) {
 case 920211:return 455;
 case 920212:return 454;
 case 920213:return 453;
 case 920214:return 455;
 case 920215:return 454;
 default:return 0;
 }
}

// Database-owned Rathe namespace reservation001. These are server-local
// identities, not claims about original Live task/template identifiers.
inline constexpr std::array<TaskBinding, 7> RatheTasks{{
	{920216, "Retinue of the Frog-God"},
	{920217, "Find Your Allies"},
	{920218, "Whom Frogs Destroy"},
	{920219, "The Wrath of Korascian"},
	{920220, "Disrupt the Siege"},
	{920221, "Champions No More"},
	{920222, "Murdunk's Last Stand"},
}};
constexpr bool IsRatheTask(uint32_t task) {
 for (const auto& bound : RatheTasks) if (bound.id == task) return true;
 return false;
}
constexpr bool IsRatheSharedTask(uint32_t task) { return task == 920222; }
constexpr bool IsTerminalSceneMission(uint32_t task) {
 return (task >= 920211 && task <= 920215) || IsRatheSharedTask(task);
}

// Exact current source/door and source/zone-point mappings for default
// returns only. Explicit A, bind, GM summon and internal Void paths are not
// default return contexts. No client-supplied zone ID alone establishes one.
constexpr bool IsDefaultVoidReturnDoor(uint32_t source, uint32_t door)
{
	return (source==219 && door==33871) || (source==466 && door==18706) ||
		(source==470 && door==18698) || (source==472 && door==18737) ||
		(source==456 && door==18761) || (source==452 && door==18818) ||
		(source==475 && door==18879);
}
constexpr bool IsDefaultVoidReturnPoint(uint32_t source, uint32_t point)
{
	return (source==466 && point==21) || (source==470 && point==26) ||
		(source==472 && point==25) || (source==456 && point==22) ||
		(source==452 && point==23) || (source==475 && point==24);
}

constexpr bool IsReservedOceangreenTask(uint32_t task) { return (task >= 920175 && task <= 920187) || task == 920190; }
constexpr bool IsOceangreenRequest(uint32_t task) { return task >= 920175 && task <= 920190; }

constexpr bool IsSodRequest(uint32_t task) { return IsOceangreenRequest(task) || IsKithicorTask(task) || IsFieldOfScaleTask(task) || IsRatheTask(task); }
constexpr bool IsBoundTask(uint32_t task) { return IsReservedOceangreenTask(task) || IsKithicorTask(task) || IsFieldOfScaleTask(task) || IsRatheTask(task); }

} // namespace SodProgression
