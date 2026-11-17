#pragma once
#include "common/database.h"
#include "common/shared_task_admission.h"
#include <cstdint>
#include <string>

// Food's semantic receipt only. The ONE common physical writer owns original
// inventory lease, intent/preimage/return blobs and actual return verification.
// Safe in Zone complete-delete and shared inventory-session recovery: no Client,
// task callback, lazy occurrence allocation or player-state reconstruction here.
namespace SodWeightedDeliveryRecovery {
struct Identity {
 uint32_t character_id=0,task_id=920198,activity_id=1,accepted_time=0,recipe_model=0;
 uint64_t occurrence_id=0;
};
enum class State { Unavailable, Missing, Prepared, Debited, Acknowledged, Returned };
inline std::string Hex(const std::string& value) {
 static constexpr char digits[]="0123456789ABCDEF";
 std::string out;out.reserve(value.size()*2);
 for(const unsigned char c:value){out.push_back(digits[c>>4]);out.push_back(digits[c&15]);}
 return out;
}
inline bool Valid(const Identity& i,const std::string& key,const std::string& context) {
 if(!i.character_id || !i.occurrence_id || !i.accepted_time || !i.recipe_model ||
  i.task_id!=920198 || i.activity_id!=1 || key.size()!=32 || context.empty())return false;
 for(const char c:key)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;
 return true;
}
// Requires caller's existing strict exclusive recovery transaction/lease. No
// current active-task join: abandonment/reaccept must not strand owed returns.
inline State Inspect(Database& db,const Identity& i,const std::string& key,const std::string& context) {
 if(!Valid(i,key,context))return State::Unavailable;
 auto r=db.QueryDatabase(fmt::format(
  "SELECT character_id,task_id,occurrence_id,activity_id,accepted_time,recipe_model,status,HEX(context_blob) "
  "FROM sod_weighted_delivery_actions WHERE request_key='{}' FOR UPDATE",key));
 if(!r.Success() || r.RowCount()>1)return State::Unavailable;
 if(r.RowCount()==0)return State::Missing;
 auto row=r.begin();
 const auto who=SharedTaskAdmission::Number(row[0]),task=SharedTaskAdmission::Number(row[1]),
  occurrence=SharedTaskAdmission::Number(row[2]),activity=SharedTaskAdmission::Number(row[3]),
  accepted=SharedTaskAdmission::Number(row[4]),model=SharedTaskAdmission::Number(row[5]),
  status=SharedTaskAdmission::Number(row[6]);
 if(!who || *who!=i.character_id || !task || *task!=i.task_id ||
  !occurrence || *occurrence!=i.occurrence_id || !activity || *activity!=i.activity_id ||
  !accepted || *accepted!=i.accepted_time || !model || *model!=i.recipe_model ||
  !status || *status>3 || !row[7] || std::string(row[7])!=Hex(context))return State::Unavailable;
 switch(*status){case 0:return State::Prepared;case 1:return State::Debited;
  case 2:return State::Acknowledged;case 3:return State::Returned;default:return State::Unavailable;}
}
// Existing physical states:0intent,1debitcommitted,2fullyreturned,3abort-only.
// An abort-only intent can precede any semantic action. It may owe all physical
// returns, but missing semantic rows never authorize an item debit or credit.
inline bool MatchesPhysical(State state,unsigned physical,bool proved_abort_only_origin=false) {
 switch(physical) {
 case 0:return state==State::Prepared;
 case 1:return state==State::Debited || state==State::Acknowledged;
 case 2:return state==State::Returned || (proved_abort_only_origin && state==State::Missing);
 case 3:return state==State::Prepared || state==State::Missing;
 default:return false;
 }
}
// Call ONLY after the common writer verifies the full physical-return outcome,
// in the SAME strict transaction that persists its physical=2/settled result.
// Never call during unknown commit resolution or for a committed debit. Caller
// retains ownership of COMMIT/ROLLBACK; a false return rolls back the whole plan.
inline bool MarkAfterVerifiedFullReturn(Database& db,const Identity& i,
 const std::string& key,const std::string& context,unsigned original_physical) {
 if(original_physical!=0 && original_physical!=3)return false;
 const auto state=Inspect(db,i,key,context);
 if(state==State::Missing)return original_physical==3;
 if(state!=State::Prepared)return false;
 auto r=db.QueryDatabase(fmt::format(
  "UPDATE sod_weighted_delivery_actions SET status=3,updated_at=UNIX_TIMESTAMP() "
  "WHERE request_key='{}' AND character_id={} AND task_id={} AND occurrence_id={} "
  "AND activity_id={} AND accepted_time={} AND recipe_model={} AND status=0 AND context_blob=x'{}'",
  key,i.character_id,i.task_id,i.occurrence_id,i.activity_id,i.accepted_time,i.recipe_model,Hex(context)));
 return r.Success() && r.RowsAffected()==1;
}
}
