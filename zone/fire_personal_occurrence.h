/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

// Feature-private native Fire support; no scripting API or recovery allocator.
#include "common/database.h"
#include "zone/tasks.h"
#include "fmt/format.h"
#include <cstdint>
#include <optional>
#include <string>

namespace FirePersonalOccurrenceSource {
struct Binding {
 uint32_t character_id;
 uint32_t task_id;
 int slot;
 int accepted_time;
 uint64_t occurrence_id;
};
struct Rule {
 uint32_t task_id, ingredient_a, ingredient_b, inactive_item, construct_npc, encounter_zone;
};
inline std::optional<Rule> OriginalRule(uint32_t task_id) {
 switch (task_id) {
 case 920167: return Rule{920167,50745,50744,50746,490922,487};
 case 920168: return Rule{920168,50747,50748,50749,490923,481};
 case 920169: return Rule{920169,50750,50751,50752,490924,490};
 default: return std::nullopt;
 }
}
inline bool MatchesLocal(const Binding& b,uint32_t character_id,TaskType type,
 const ClientTaskInformation& current) {
 return OriginalRule(b.task_id).has_value() && b.character_id!=0 &&
  b.occurrence_id!=0 && b.character_id==character_id && type==TaskType::Quest &&
  current.task_id==static_cast<int>(b.task_id) && current.slot==b.slot &&
  current.accepted_time==b.accepted_time && !current.was_rewarded;
}
// Fresh persisted state, not an inventory/actor commit. Never cache a true result across operations.
inline std::optional<bool> PersistedMatches(Database& db,const Binding& b,bool initial_acceptance) {
 if (!OriginalRule(b.task_id) || !b.character_id || !b.occurrence_id || b.accepted_time<=0 || b.slot<0)
  return false;
 const std::string initial=initial_acceptance ?
  " AND (SELECT COUNT(*) FROM character_activities a WHERE a.charid=ct.charid "
  "AND a.taskid=ct.taskid AND a.activityid BETWEEN 0 AND 3 "
  "AND a.donecount=0 AND a.completed=0)=4" : "";
 auto result=db.QueryDatabase(fmt::format(
  "SELECT r.occurrence_id FROM character_task_reward_instances r "
  "JOIN character_tasks ct ON ct.charid=r.character_id AND ct.taskid=r.task_id "
  "AND ct.acceptedtime=r.accepted_time "
  "WHERE r.character_id={} AND r.task_id={} AND r.occurrence_id={} "
  "AND r.accepted_time={} AND ct.slot={} AND ct.type=2 AND ct.was_rewarded=0{}",
  b.character_id,b.task_id,b.occurrence_id,b.accepted_time,b.slot,initial));
 if (!result.Success() || result.RowCount()>1) return std::nullopt;
 if (result.RowCount()==0) return false;
 auto row=result.begin();
 return row[0] && std::string(row[0])==std::to_string(b.occurrence_id);
}
// actual_save_result is ONLY the actual native SaveClientState return from this acceptance.
// fresh_id is ONLY the actual successful ReserveTaskRewardInstance result from this acceptance.
// Neither parameter comes from Lua, a packet, a historical lookup or an invented getter.
inline std::optional<Binding> BindAccepted(Database& db,uint32_t character_id,TaskType type,
 const ClientTaskInformation& actual,uint64_t fresh_id,bool actual_save_result) {
 if (!actual_save_result || !OriginalRule(static_cast<uint32_t>(actual.task_id))) return std::nullopt;
 Binding b{character_id,static_cast<uint32_t>(actual.task_id),actual.slot,actual.accepted_time,fresh_id};
 if (!MatchesLocal(b,character_id,type,actual)) return std::nullopt;
 auto matched=PersistedMatches(db,b,true);
 return matched && *matched ? std::optional<Binding>(b) : std::nullopt;
}
enum class Consumer { CombineIngredients, ActivateConstruct, ActualConstructDeath, FinalHail };
struct Context {
 Binding acceptance;
 Rule rule;
 Consumer consumer;
 int activity;
};
// Produces acceptance/stage context ONLY. Not permission to debit, summon, spawn, credit or reward.
inline std::optional<Context> ForConsumer(Database& db,const Binding& binding,
 uint32_t actual_character_id,TaskType actual_type,const ClientTaskInformation& current,
 Consumer consumer) {
 if (!MatchesLocal(binding,actual_character_id,actual_type,current)) return std::nullopt;
 auto persisted=PersistedMatches(db,binding,false);
 if (!persisted || !*persisted) return std::nullopt;
 const int activity=consumer==Consumer::FinalHail ? 3 : 2;
 if (current.activity[activity].activity_state!=ActivityActive) return std::nullopt;
 auto rule=OriginalRule(binding.task_id);
 if (!rule) return std::nullopt;
 return Context{binding,*rule,consumer,activity};
}
} // namespace FirePersonalOccurrenceSource
