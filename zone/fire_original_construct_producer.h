// SOURCE ONLY: original transient construct producer; no registration/runtime authority.
#pragma once
#include "zone/entity.h"
#include "zone/client.h"
#include "zone/npc.h"
#include "fire_personal_item_use.h"
#include "fire_personal_construct_identity.h"
#include "fire_original_final_hail_frame.h"
#include <cmath>
#include <array>
#include "fire_prepared_construct_plan.h"
#include "fire_construct_participants.h"
namespace FirePersonalOccurrenceSource {
enum class OriginalConstructDeathResult { Hold, SubmittedWithoutCommitReceipt, CommittedAtOriginalSave };
class OriginalConstructAttempt {
 friend struct OriginalConstructRegistration;
 friend void CaptureConstructBeforeOriginalSpawnCallbacks(NPC*);
 friend class PublishedNativeUseSink;
 friend class PreparedConstructPlan;
 friend class NativeConstructParticipants;
 friend class OriginalConstructNativeDeath;
 friend class ConstructDeathReceiptStore;
 friend class OriginalConstructDeathQueue;
 friend class OriginalConstructNativeHail;
 OriginalItemUse original_;
 uint64_t native_click_sequence_=0;
 uint32_t activation_group_id_=0;
 std::array<ActivationParticipant,6> activation_cohort_{};
 std::array<NativeConstructParticipantVisit,6> native_visits_{};
 size_t activation_count_=0;
 bool persisted_death_=false;
 bool attempted_=false,caster_death_submitted_=false,caster_death_committed_=false,death_entry_active_=false,death_success_sealed_=false;
 NPC* original_object_=nullptr; // only compare during genuine live native death
 std::optional<ConstructIdentity> captured_;
public:
 explicit OriginalConstructAttempt(const OriginalItemUse& original):original_(original) {}
 OriginalConstructAttempt(const OriginalConstructAttempt&)=delete;
 OriginalConstructAttempt& operator=(const OriginalConstructAttempt&)=delete;
};
struct OriginalConstructRegistration {
 OriginalConstructAttempt& attempt;
 NPC* expected_object;
 uint32_t actual_zone,actual_instance;
 bool entered=false,captured_before_callbacks=false;
};
inline thread_local OriginalConstructRegistration* original_construct_registration=nullptr;
class OriginalConstructRegistrationScope {
 OriginalConstructRegistration* previous_;
public:
 explicit OriginalConstructRegistrationScope(OriginalConstructRegistration& f)
  :previous_(original_construct_registration) { original_construct_registration=&f; }
 ~OriginalConstructRegistrationScope() { original_construct_registration=previous_; }
 OriginalConstructRegistrationScope(const OriginalConstructRegistrationScope&)=delete;
 OriginalConstructRegistrationScope& operator=(const OriginalConstructRegistrationScope&)=delete;
};
// Native AddNPC seam: AFTER its actual ID/list insertion, BEFORE ScanCloseMobs/EVENT_SPAWN.
inline void CaptureConstructBeforeOriginalSpawnCallbacks(NPC* actual_registered) {
 auto* f=original_construct_registration;
 if(!f || f->entered || actual_registered!=f->expected_object) return;
 f->entered=true;
 auto captured=CaptureActualReturnedConstruct(f->attempt.original_,*actual_registered,
  f->actual_zone,f->actual_instance);
 if(!captured) return;
 f->attempt.original_object_=actual_registered;
 f->attempt.captured_=*captured;
 f->captured_before_callbacks=true;
}
struct OriginalConstructDeathFrame {
 Binding original;
 Client* client;
 bool entered=false,save_observed=false,committed=false;
};
inline thread_local OriginalConstructDeathFrame* original_construct_death_frame=nullptr;
class OriginalConstructDeathScope {
 OriginalConstructDeathFrame* previous_;
public:
 explicit OriginalConstructDeathScope(OriginalConstructDeathFrame& f)
  :previous_(original_construct_death_frame) { original_construct_death_frame=&f; }
 ~OriginalConstructDeathScope() { original_construct_death_frame=previous_; }
 OriginalConstructDeathScope(const OriginalConstructDeathScope&)=delete;
 OriginalConstructDeathScope& operator=(const OriginalConstructDeathScope&)=delete;
};
inline OriginalConstructDeathFrame* ClaimOriginalConstructDeathUpdate(Client* client,TaskType type,
 const ClientTaskInformation& current,int goal,int count) {
 auto* f=original_construct_death_frame;
 if(!f || f->entered || f->client!=client || goal!=2 || count!=1 ||
  !MatchesLocal(f->original,client->CharacterID(),type,current) ||
  current.activity[2].activity_state!=ActivityActive) return nullptr;
 f->entered=true;return f;
}
inline void ObserveOriginalConstructDeathSave(OriginalConstructDeathFrame* f,Database& db,Client* client,
 TaskType type,const ClientTaskInformation& current,bool actual_existing_save_result) {
 if(!f || f->save_observed || f->client!=client) return;
 f->save_observed=true;
 if(!actual_existing_save_result || !MatchesLocal(f->original,client->CharacterID(),type,current) ||
  current.activity[2].activity_state!=ActivityCompleted ||
  current.activity[3].activity_state!=ActivityActive) return;
 auto matched=PersistedMatches(db,f->original,false);
 if(!matched || !*matched) return;
 auto row=db.QueryDatabase(fmt::format(
  "SELECT 1 FROM character_activities WHERE charid={} AND taskid={} "
  "AND activityid=2 AND donecount=1 AND completed=1",
  f->original.character_id,f->original.task_id));
 f->committed=row.Success() && row.RowCount()==1;
}
// LOCAL168/169 cohort is captured prespend; actual native candidate visit, sealed death,
// same-save observer and recipient-owned occurrence are all required.167 remains caster-only.
} // namespace FirePersonalOccurrenceSource
