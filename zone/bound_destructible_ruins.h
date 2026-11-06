#pragma once

#include "npc.h"
#include "spawn2.h"

// Explicit server lifecycle policy for reviewed, bound, lootless destructibles.
// No Mob/NPC/Spawn2 layout changes or new client opcode are required.
namespace BoundDestructibles {

inline bool IsRuins(NPC *npc)
{
	return npc && npc->IsDestructibleObject() && npc->GetSpawn() && npc->GetSpawnPointID() &&
		npc->GetEntityVariable("destructible_retain_ruins") == "1" &&
		!npc->GetEntityVariable("destructible_zone_actor").empty() &&
		!npc->GetEntityVariable("destructible_map_model").empty() &&
		npc->GetEntityVariable("destructible_ruins") == "1";
}

inline bool CanRetain(NPC *npc)
{
	return npc && npc->IsDestructibleObject() && npc->GetSpawn() && npc->GetSpawnPointID() &&
		!npc->HasOwner() && !npc->IsMultiQuestEnabled() && !npc->IsMerc() && !npc->GetSwarmInfo() && !npc->IsQueuedForCorpse() &&
		npc->GetEntityVariable("destructible_retain_ruins") == "1" &&
		!npc->GetEntityVariable("destructible_zone_actor").empty() &&
		!npc->GetEntityVariable("destructible_map_model").empty() &&
		npc->GetLoottableID() == 0 && npc->GetLootItems().empty() &&
		npc->GetCopper() == 0 && npc->GetSilver() == 0 && npc->GetGold() == 0 && npc->GetPlatinum() == 0;
}

inline bool IsTransitioning(NPC *npc)
{
	return npc && npc->GetEntityVariable("destructible_death_transition") == "1";
}

inline bool IsBlocked(NPC *npc)
{
	return IsTransitioning(npc) || IsRuins(npc);
}

inline void CancelTransition(NPC *npc)
{
	if (IsTransitioning(npc)) {
		npc->SetEntityVariable("destructible_death_cancelled", "1");
	}
}

class DeathTransition {
public:
	explicit DeathTransition(NPC *npc) : npc_(npc), spawn_(npc->GetSpawn()), active_(CanRetain(npc) && !npc->GetDepop())
	{
		if (active_) {
			npc_->SetEntityVariable("destructible_death_transition", "1");
			npc_->DeleteEntityVariable("destructible_death_cancelled");
		}
	}

	~DeathTransition()
	{
		if (!active_) {
			return;
		}
		// Only our temporary DeathReset attachment is ours to release. A
		// callback-created replacement/reset is never reattached or overwritten.
		if (reset_performed_ && !IsRuins(npc_) && spawn_->GetNPC() == npc_) {
			spawn_->SetNPCPointerNull();
			spawn_->SetCurrentNPCID(0);
		}
		npc_->DeleteEntityVariable("destructible_death_transition");
		npc_->DeleteEntityVariable("destructible_death_cancelled");
	}

	DeathTransition(const DeathTransition&) = delete;
	DeathTransition& operator=(const DeathTransition&) = delete;
	bool Active() const { return active_; }
	bool Cancelled() const { return active_ && npc_->GetEntityVariable("destructible_death_cancelled") == "1"; }
	void AfterDeathReset() { reset_performed_ = active_; }
	bool CanFinish() const
	{
		return active_ && !Cancelled() && !npc_->GetDepop() && npc_->GetSpawn() == spawn_ &&
			spawn_->GetNPC() == npc_ && CanRetain(npc_);
	}

private:
	NPC *npc_;
	Spawn2 *spawn_;
	bool active_;
	bool reset_performed_ = false;
};

inline void EnterRuins(NPC *npc)
{
	npc->SetEntityVariable("destructible_ruins", "1");
	npc->AI_Stop();
	npc->SetHP(0);
	npc->SetTarget(nullptr);
	if (npc->GetID()) {
		npc->SetTargetable(false);
		npc->SendHPUpdate(); // Existing damage-state4; never a corpse/delete.
	}
}

inline bool SavedRuins(const std::map<std::string, std::string> &variables)
{
	const auto value = [&](const char *key) {
		const auto it = variables.find(key);
		return it == variables.end() ? std::string() : it->second;
	};
	return value("destructible_ruins") == "1" && value("destructible_retain_ruins") == "1" &&
		!value("destructible_zone_actor").empty() && !value("destructible_map_model").empty();
}

} // namespace BoundDestructibles
