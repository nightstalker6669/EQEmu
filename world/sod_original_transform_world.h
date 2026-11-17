#pragma once
#include "common/sod_original_transform_native.h"
#include "common/shared_core/original_trade_custody.h"
#include "common/repositories/shared_task_activity_state_repository.h"
#include <array>

namespace SodOriginalTransformWorld {
namespace S=SodOriginalTransform;
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
// Scope the EXISTING canonical shared activity saver, rather than create a
// parallel native activity persistence path. Its owner transaction also owns
// actor lease -> physical request -> native run/decision locks in that order.
class Save final {
 Database& db_;const std::string& key_;const X::Context& context_;const X::Decision& decision_;
 bool saved_=false;Save* previous_=nullptr;
 inline static thread_local Save* current_=nullptr;
public:
 Save(Database& db,const std::string& key,const X::Context& c,const X::Decision& d)
 :db_(db),key_(key),context_(c),decision_(d),previous_(current_){current_=this;}
 ~Save(){current_=previous_;}
 Save(const Save&)=delete;Save& operator=(const Save&)=delete;
 bool Saved()const{return saved_;}
 static bool Selected(uint64_t run){return current_&&current_->context_.occurrence==run;}
 static void CommitActualSave(Database& db,uint64_t run,
  const std::vector<SharedTaskActivityStateRepository::SharedTaskActivityState>& rows) {
  auto* f=current_;
  if(!f||&db!=&f->db_||f->saved_||!f->context_.shared||f->context_.task!=920197||run!=f->context_.occurrence||rows.size()!=4)
   throw std::runtime_error("Original World selected save differs");
  S::Strict(db);std::array<bool,4> seen{};
  for(const auto& a:rows){
   if(a.shared_task_id!=int64_t(run)||a.activity_id<0||a.activity_id>3||seen[a.activity_id]||a.done_count<0||
    a.done_count>(a.activity_id==0?4:1))throw std::runtime_error("Original World activity snapshot differs");
   seen[a.activity_id]=true;
   if((a.activity_id==0&&uint32_t(a.done_count)!=f->decision_.tail_target)||
    (a.activity_id==1&&uint32_t(a.done_count)!=f->decision_.primary_target))
    throw std::runtime_error("Original World accepted target differs");
  }
  auto removed=S::Q(db,fmt::format("DELETE FROM shared_task_activity_state WHERE shared_task_id={}",run));
  if(removed.RowsAffected()!=rows.size()||SharedTaskActivityStateRepository::InsertMany(db,rows)!=int(rows.size())||db.TransactionStrictFailed())
   throw std::runtime_error("Original World selected native save failed");
  S::InsertFinal(db,f->key_,f->context_,f->decision_);f->saved_=true;
 }
};
}
