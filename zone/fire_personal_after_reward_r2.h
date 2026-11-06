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
#include "fire_personal_occurrence.h"
#include <charconv>

namespace FirePersonalOccurrenceSource {
struct NextIntent {
 Binding finished;
 uint64_t pending_reward_id;
 uint32_t next_task;
 TaskType next_type;
};
inline uint32_t NextTask(uint32_t finished) {
 switch(finished) { case 920167:return 920168; case 920168:return 920169; case 920169:return 920170; }
 return 0;
}
inline uint64_t OriginalCurrencyReward(uint32_t task) {
 switch(task) { case 920167:return 3839201671ULL; case 920168:return 3839201681ULL;
  case 920169:return 3839201691ULL; default:return 0; }
}
// Called only by the original physical-final-hail orchestrator AFTER its real RewardTask succeeds,
// BEFORE native removal invalidates current/active pointer. Never from TASK_COMPLETE or a generic retry loop.
struct ActualOriginalRewardCall {
 bool executed=false;
 bool was_rewarded_before_call=true;
 bool result=false;
};
inline std::optional<NextIntent> AfterOriginalRewardTask(Database& db,const Binding& finished,
 uint32_t actual_character_id,TaskType actual_type,const ClientTaskInformation& current,
 const ActualOriginalRewardCall& actual_call) {
 if (!actual_call.executed || actual_call.was_rewarded_before_call || !actual_call.result || actual_type!=TaskType::Quest || !finished.occurrence_id ||
  !finished.character_id || actual_character_id!=finished.character_id ||
  current.task_id!=static_cast<int>(finished.task_id) || current.slot!=finished.slot ||
  current.accepted_time!=finished.accepted_time || !current.was_rewarded || !NextTask(finished.task_id))
  return std::nullopt;
 for(int activity=0;activity<4;++activity)
  if(current.activity[activity].activity_state!=ActivityCompleted) return std::nullopt;
 const uint64_t original_currency_reward=OriginalCurrencyReward(finished.task_id);
 auto result=db.QueryDatabase(fmt::format(
  "SELECT s.pending_reward_id FROM character_task_reward_selections s "
  "JOIN character_task_reward_instances r ON r.character_id=s.character_id "
  "AND r.task_id=s.task_id AND r.occurrence_id=s.source_instance_id "
  "WHERE s.character_id={} AND s.task_id={} AND s.accepted_time={} "
  "AND r.accepted_time={} AND s.source_instance_id={} AND s.reward_set_id={} AND s.status=1 "
  "AND EXISTS(SELECT 1 FROM character_task_rewards e "
  "WHERE e.character_id=s.character_id AND e.pending_reward_id=s.pending_reward_id "
  "AND e.reward_id={} AND e.status=1)",
  finished.character_id,finished.task_id,finished.accepted_time,finished.accepted_time,
  finished.occurrence_id,finished.task_id,original_currency_reward));
 if(!result.Success() || result.RowCount()!=1) return std::nullopt;
 auto row=result.begin();
 if(!row[0]) return std::nullopt;
 // Existing native typed parser; no floating conversion of occurrence/pending identities.
 const std::string pending_text(row[0]);
 uint64_t pending=0;
 const auto parsed=std::from_chars(pending_text.data(),pending_text.data()+pending_text.size(),pending);
 if(parsed.ec!=std::errc{} || parsed.ptr!=pending_text.data()+pending_text.size() || !pending)
  return std::nullopt;
 return NextIntent{finished,pending,NextTask(finished.task_id),
  finished.task_id==920169 ? TaskType::Shared : TaskType::Quest};
}
// Required private observations of the SAME original native completion/cleanup call.
// These are NOT new callback fields/getters and are not available until the native owner captures them.
struct ActualOriginalCleanup {
 uint32_t character=0,finished_task=0;
 uint64_t original_occurrence=0;
 int original_slot=-1;
 bool original_removal_executed=false;
 bool task_and_activity_removal_known=false;
 bool original_pointer_removal_known=false;
 bool original_slot_now_free=false;
 bool original_completion_save_executed=false;
 bool original_completion_save_succeeded=false;
};
class Handoff {
 NextIntent intent_;
 bool cleanup_known_=false,attempted_=false,personal_accepted_=false;
 uint64_t next_personal_occurrence_=0;
 friend bool ConfirmOriginalCleanup(Handoff&,const ActualOriginalCleanup&);
 friend std::optional<NextIntent> TakeOriginalHandoffOnce(Handoff&);
 friend bool RecordActualPersonalAcceptance(Handoff&,const Binding&);
public:
 explicit Handoff(const NextIntent& original):intent_(original) {}
 Handoff(const Handoff&)=delete;
 Handoff& operator=(const Handoff&)=delete;
 Handoff(Handoff&&)=delete;
 Handoff& operator=(Handoff&&)=delete;
};
inline bool ConfirmOriginalCleanup(Handoff& handoff,const ActualOriginalCleanup& actual) {
 const auto& old=handoff.intent_.finished;
 if(handoff.attempted_ || actual.character!=old.character_id || actual.finished_task!=old.task_id ||
  actual.original_occurrence!=old.occurrence_id || actual.original_slot!=old.slot ||
  !actual.original_removal_executed || !actual.task_and_activity_removal_known ||
  !actual.original_pointer_removal_known || !actual.original_slot_now_free ||
  !actual.original_completion_save_executed || !actual.original_completion_save_succeeded) return false;
 handoff.cleanup_known_=true;return true;
}
inline std::optional<NextIntent> TakeOriginalHandoffOnce(Handoff& handoff) {
 if(!handoff.cleanup_known_ || handoff.attempted_) return std::nullopt;
 handoff.attempted_=true; // Before the native owner's ONE actual successor dispatch.
 return handoff.intent_;
}
inline bool RecordActualPersonalAcceptance(Handoff& handoff,const Binding& actual_next) {
 if(!handoff.attempted_ || handoff.personal_accepted_ || handoff.intent_.next_type!=TaskType::Quest ||
  actual_next.character_id!=handoff.intent_.finished.character_id ||
  actual_next.task_id!=handoff.intent_.next_task || !actual_next.occurrence_id ||
  actual_next.occurrence_id==handoff.intent_.finished.occurrence_id) return false;
 handoff.personal_accepted_=true;
 handoff.next_personal_occurrence_=actual_next.occurrence_id;
 return true;
}
// No assignment, World request or invented World-success receipt is provided.
// Native owner retains ONE object per original physical finish. Reconstructing another resets state and is forbidden.
} // namespace FirePersonalOccurrenceSource
