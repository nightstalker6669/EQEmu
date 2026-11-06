#include "fungal_item_world_source.h"
#include "../common/fungal_item_custody_sql.h"
#include "../common/fungal_world_hold.h"
#include "../common/shared_tasks.h"
#include <map>
#include <algorithm>
#include <memory>
#include <stdexcept>
namespace FungalInventory {
namespace {
using namespace FungalCustodySQL;
class Transaction final:public WorldTransaction {
public:
 Database* db=nullptr;
 const FungalItemWorldSource::OriginalWorldUpdate* original=nullptr;
 uint64_t connection=0,run=0,outcome=0;
 bool active=false,known_commit=false,commit_attempted=false,already_ack=false,pure=false;
 std::map<int,Activity> before;
 std::unique_ptr<WorldHold> unknown;
 ~Transaction(){if(active&&db)db->TransactionRollbackStrict();}
};
std::string Time(time_t t){return t>0?"FROM_UNIXTIME("+std::to_string(t)+")":"NULL";}
}
std::unique_ptr<WorldTransaction> WorldCustody::Prepare(Database& db,const FungalItemWorldSource::OriginalWorldUpdate& original) {
 const auto& f=original.original_;auto* actual=original.actual_;
 const bool pure=(original.activity_==0||original.activity_==1||original.activity_==6)&&f.outcome==0;
 if((!pure&&!FungalItemWire::Selected(920143,original.activity_))||original.count_!=(pure?1u:(original.activity_==5?2u:1u))||!actual||!WorldHold::Allows(f.run))return nullptr;
 const auto& current=actual->GetDbSharedTask();
 if(current.id<=0||uint64_t(current.id)!=f.run||current.task_id!=920143||current.accepted_time!=f.accepted||current.completion_time||(!pure&&!f.outcome)||!f.recipient)return nullptr;
 auto t=std::make_unique<Transaction>();t->db=&db;t->original=&original;t->run=f.run;t->outcome=f.outcome;t->pure=pure;t->unknown=std::unique_ptr<WorldHold>(new WorldHold(f.run));
 try {
  auto connection=Query(db,"SELECT CONNECTION_ID(),@@in_transaction,@@autocommit");
  if(connection.RowCount()!=1||Number(connection.begin()[1])||Number(connection.begin()[2])!=1)return nullptr;t->connection=Number(connection.begin()[0]);
  if(!db.TransactionBeginStrict().Success())return nullptr;t->active=true;
  t->before=LockShared(db,f.run,f.accepted,f.recipient,original.activity_,true);
  const uint32_t goal=pure?1:(original.activity_==5?2:1);
  if(!pure){
  auto outcome=Query(db,"SELECT native_ack,output_item,output_quantity FROM fungal_item_operations WHERE outcome_id="+std::to_string(f.outcome)+" AND scope=2 AND task_id=920143 AND native_occurrence="+std::to_string(f.run)+" AND activity_id="+std::to_string(original.activity_)+" AND accepted_time="+std::to_string(f.accepted)+" AND recipient_character="+std::to_string(f.recipient)+" FOR UPDATE");
  if(outcome.RowCount()!=1)return nullptr;
  auto row=outcome.begin();
  if(Number(row[1])!=(original.activity_==5?43586:43587)||Number(row[2])!=goal)return nullptr;
  t->already_ack=Number(row[0])==1;
  }
  const auto& stage=t->before.at(original.activity_);
  if((pure&&(stage.count||stage.completed))||(!pure&&((!t->already_ack&&(stage.count||stage.completed))||(t->already_ack&&(stage.count!=int(goal)||!stage.completed)))))return nullptr;
  // DB locks are held BEFORE Source mutates this very native SharedTask vector.
  const auto native=actual->GetActivityState();
  if(native.size()!=t->before.size())return nullptr;
  if(pure){
   if(native.size()!=12)return nullptr;
   const auto active=Tasks::GetActiveElements(native,native,native.size());
   const auto at=std::find_if(native.begin(),native.end(),[&](const auto& a){return a.activity_id==original.activity_;});
   if(at==native.end()||active.is_task_complete||std::find(active.active.begin(),active.active.end(),int(at-native.begin()))==active.active.end())return nullptr;
  }
  for(const auto& a:native){if(pure&&a.activity_id==original.activity_&&a.max_done_count!=1)return nullptr;auto at=t->before.find(a.activity_id);if(at==t->before.end()||a.done_count!=uint32_t(at->second.count)||a.updated_time!=at->second.updated||a.completed_time!=at->second.completed)return nullptr;}
  return t;
 }catch(...){return nullptr;}
}
std::unique_ptr<FungalItemWorldSource::KnownStageAck> WorldCustody::CommitActualSave(Database& db,WorldTransaction& base,const FungalItemWorldSource::OriginalWorldUpdate& original,const std::vector<SharedTaskActivityStateRepository::SharedTaskActivityState>& rows) {
 auto* t=dynamic_cast<Transaction*>(&base);
 if(!t||!t->active||t->db!=&db||t->original!=&original||!WorldHold::Allows(t->run))return nullptr;
 try {
  const auto& f=original.original_;const auto native=original.actual_->GetActivityState();
  if(rows.size()!=t->before.size()||native.size()!=rows.size())throw std::runtime_error("Fungal native snapshot universe differs");
  std::map<int,SharedTaskActivityStateRepository::SharedTaskActivityState> actual;
  for(const auto& a:rows)if(a.shared_task_id!=int64_t(f.run)||a.done_count<0||a.updated_time<0||a.completed_time<0||!actual.emplace(a.activity_id,a).second)throw std::runtime_error("Fungal native snapshot identity differs");
  for(const auto& n:native){auto at=actual.find(n.activity_id);if(at==actual.end()||n.done_count!=uint32_t(at->second.done_count)||n.updated_time!=uint64_t(at->second.updated_time)||n.completed_time!=uint64_t(at->second.completed_time))throw std::runtime_error("Fungal snapshot is not actual native vector");}
  for(const auto& before:t->before){const auto at=actual.find(before.first);if(at==actual.end())throw std::runtime_error("Fungal original stage missing");const auto& after=at->second;
   if(uint32_t(before.first)==original.activity_){if(after.done_count!=int(original.count_)||after.completed_time<=0||after.updated_time<=0)throw std::runtime_error("Fungal original selected result differs");}
   else if(after.done_count!=before.second.count||uint64_t(after.updated_time)!=before.second.updated||uint64_t(after.completed_time)!=before.second.completed)throw std::runtime_error("Fungal other native stage changed");
  }
  // Private ACK is preallocated before any persistent mutation or COMMIT.
  auto ack=std::unique_ptr<FungalItemWorldSource::KnownStageAck>(new FungalItemWorldSource::KnownStageAck(f,original.activity_,original.count_));
  auto state=Query(db,"SELECT CONNECTION_ID(),@@in_transaction");if(state.RowCount()!=1||Number(state.begin()[0])!=t->connection||Number(state.begin()[1])!=1||db.TransactionStrictFailed())throw std::runtime_error("Fungal World strict session differs");
  // Persist precisely the existing native full snapshot at its original Save
  // call, not a standalone replacement activity/progress UPDATE.
  auto removed=Query(db,"DELETE FROM shared_task_activity_state WHERE shared_task_id="+std::to_string(f.run));
  if(removed.RowsAffected()!=rows.size())throw std::runtime_error("Fungal original activity universe drift");
  for(const auto& a:rows){auto inserted=Query(db,"INSERT INTO shared_task_activity_state(shared_task_id,activity_id,done_count,updated_time,completed_time)VALUES("+std::to_string(a.shared_task_id)+","+std::to_string(a.activity_id)+","+std::to_string(a.done_count)+","+Time(a.updated_time)+","+Time(a.completed_time)+")");if(inserted.RowsAffected()!=1)throw std::runtime_error("Fungal actual snapshot save failed");}
  if(!t->pure&&!t->already_ack){auto acked=Query(db,"UPDATE fungal_item_operations SET native_ack=1 WHERE outcome_id="+std::to_string(f.outcome)+" AND native_ack=0");if(acked.RowsAffected()!=1)throw std::runtime_error("Fungal exact outcome ACK drift");}
  state=Query(db,"SELECT CONNECTION_ID(),@@in_transaction");if(state.RowCount()!=1||Number(state.begin()[0])!=t->connection||Number(state.begin()[1])!=1||db.TransactionStrictFailed())throw std::runtime_error("Fungal World session changed before COMMIT");
  t->commit_attempted=true;auto commit=db.TransactionCommitStrict();t->active=false;
  if(!commit.Success())throw std::runtime_error("Fungal World COMMIT unknown");t->known_commit=true;
  return ack;
 }catch(...){if(t->active){db.TransactionRollbackStrict();t->active=false;}if(t->commit_attempted&&!t->known_commit&&t->unknown){t->unknown->Attach();t->unknown.release();}return nullptr;}
}
}
