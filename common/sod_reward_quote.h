#pragma once
#include "common/database.h"
#include "common/sod_progression.h"
#include "common/shared_task_admission.h"
#include <algorithm>
#include <optional>

// Approved local reconstruction, not recovered Sony reward arithmetic.
// No separate entitlement or reward engine: these are frozen inputs to the
// native task provider and a count of acknowledged native completions.
namespace SodRewardQuote {
inline constexpr uint32_t Model = 2;
struct Bounds { uint32_t floor; uint32_t maximum; };
inline Bounds Range(uint32_t task) {
 switch(task) {
 case 920175: case 920176:return {5,15};
 case 920177:return {15,15};
 case 920178: case 920179:return {15,60};
 case 920180:return {45,180};
 case 920181:return {45,55};
 case 920182: case 920190:return {15,45};
 case 920183:return {30,60};
 // User-approved Kithicor reconstruction: same frozen popularity model,
 // guide-sourced caps. Integer quarter-cap floors are local numeric policy.
 case 920191:return {50,200};
 case 920192:return {22,90};
 case 920193:case 920194:return {11,45};
 case 920195:case 920196:return {15,60};
 // FoS LOCAL reconstruction approved for exact proposal-r1 values.
 case 920197:return {45,360};
 case 920198:return {15,45};
 case 920199:return {15,60};
 case 920200:return {15,45};
 case 920201:return {15,45};
 case 920202:return {15,45};
 case 920203:return {15,45};
 case 920204:return {10,25};
 case 920205:return {15,45};
 case 920206:return {35,140};
 case 920207:return {15,60};
 case 920208:return {15,45};
 case 920209:return {5,15};
 case 920210:return {15,45};
 case 920211:return {5,17};
 case 920212:return {5,22};
 case 920213:return {5,22};
 case 920214:return {5,25};
 case 920215:return {5,15};
 default:return {0,0};
 }
}
inline bool Shared(uint32_t task) { return task==920178||task==920179||task==920180||task==920183||task==920192||task==920193||SodProgression::IsFieldOfScaleSharedTask(task); }
inline uint32_t Amount(Bounds b,uint64_t count) {
 // Saturate before arithmetic; values beyond this already reach every floor.
 return std::max(b.floor,static_cast<uint32_t>(uint64_t(b.maximum)*8/(8+std::min<uint64_t>(count,UINT32_MAX))));
}
struct Quote { uint32_t amount; uint32_t model; uint64_t count; };
inline std::optional<Quote> Read(Database& db,uint32_t task,uint8_t kind,uint64_t occurrence) {
 if(!Range(task).maximum||!occurrence||kind!=(Shared(task)?1:0))return std::nullopt;
 auto q=db.QueryDatabase(fmt::format("SELECT amount,model_version,popularity_count FROM sod_task_reward_quotes WHERE task_id={} AND occurrence_kind={} AND occurrence_id={}",task,kind,occurrence));
 if(!q.Success()||q.RowCount()!=1)return std::nullopt;
 auto row=q.begin();auto a=SharedTaskAdmission::Number(row[0]),m=SharedTaskAdmission::Number(row[1]),c=SharedTaskAdmission::Number(row[2]);
 const auto b=Range(task);
 if(!a||!m||!c||*m!=Model||*a!=Amount(b,*c))return std::nullopt;
 return Quote{static_cast<uint32_t>(*a),static_cast<uint32_t>(*m),*c};
}
inline std::optional<Quote> Preview(Database& db,uint32_t task) {
 const auto bounds=Range(task);if(!bounds.maximum)return std::nullopt;
 auto r=db.QueryDatabase(fmt::format("SELECT COUNT(*) FROM sod_task_popularity_events WHERE task_id={} AND completed_at>UNIX_TIMESTAMP()-86400 AND completed_at<=UNIX_TIMESTAMP()",task));
 if(!r.Success()||r.RowCount()!=1)return std::nullopt;
 auto count=SharedTaskAdmission::Number(r.begin()[0]);if(!count)return std::nullopt;
 return Quote{Amount(bounds,*count),Model,*count};
}
// Caller supplies native personal allocator ID or canonical shared run ID.
// Invoke inside World admission transaction for shared; before personal task
// acceptance. INSERT-only key preserves the first quote across late joins.
inline bool Freeze(Database& db,uint32_t task,uint8_t kind,uint64_t occurrence,uint32_t accepted) {
 const auto b=Range(task);if(!b.maximum)return true;
 if(!occurrence||!accepted||kind!=(Shared(task)?1:0))return false;
 auto r=db.QueryDatabase(fmt::format(
  "INSERT INTO sod_task_reward_quotes (task_id,occurrence_kind,occurrence_id,accepted_at,model_version,popularity_count,amount) "
  "SELECT {},{},{},{},{},n,GREATEST({},FLOOR({}*8/(8+n))) FROM "
  "(SELECT COUNT(*) n FROM sod_task_popularity_events WHERE task_id={} AND completed_at>{} AND completed_at<={}) c "
  "ON DUPLICATE KEY UPDATE occurrence_id=occurrence_id",task,kind,occurrence,accepted,Model,b.floor,b.maximum,task,accepted>86400?accepted-86400:0,accepted));
 return r.Success()&&Read(db,task,kind,occurrence).has_value();
}
// Must be in the SAME strict transaction as native history/archive. A
// rollback cannot publish a popularity event; unique canonical run key keeps
// six reward recipients from counting as six completions.
inline bool RecordCompletion(Database& db,uint32_t task,uint8_t kind,uint64_t occurrence,uint32_t completed) {
 if(!Range(task).maximum)return true;
 if(!completed||!Read(db,task,kind,occurrence))return false;
 return db.QueryDatabase(fmt::format(
  "INSERT INTO sod_task_popularity_events (task_id,occurrence_kind,occurrence_id,completed_at) VALUES ({},{},{},{}) "
  "ON DUPLICATE KEY UPDATE occurrence_id=occurrence_id",task,kind,occurrence,completed)).Success();
}
inline std::optional<Quote> ForRecipient(Database& db,uint32_t character,uint32_t task,uint64_t personal) {
 uint64_t canonical=personal;
 if(Shared(task)) {
  auto r=db.QueryDatabase(fmt::format("SELECT shared_task_id FROM character_shared_task_reward_instances WHERE character_id={} AND task_id={} AND occurrence_id={}",character,task,personal));
  if(!r.Success()||r.RowCount()!=1)return std::nullopt;
  auto id=SharedTaskAdmission::Number(r.begin()[0]);if(!id||!*id)return std::nullopt;canonical=*id;
 }
 return Read(db,task,Shared(task)?1:0,canonical);
}
} // namespace SodRewardQuote
