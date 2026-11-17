#pragma once
#include "shared_core/original_transform_native.h"
#include "database.h"
#include "shared_task_admission.h"
#include "strings.h"
#include <algorithm>
#include <stdexcept>

// Spells' semantic adapter for Core's ONE original physical writer. The final
// decision is not a second property journal; the original row owns all blobs.
namespace SodOriginalTransform {
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
inline constexpr uint32_t Model=1;
inline bool Key(const std::string& s) {
 return s.size()==32 && std::all_of(s.begin(),s.end(),[](unsigned char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
inline std::string Hex(const std::string& s) {
 static constexpr char h[]="0123456789abcdef";std::string out;out.reserve(s.size()*2);
 for(unsigned char c:s){out+=h[c>>4];out+=h[c&15];}return out;
}
inline std::string Unhex(const char* s) {
 if(!s)throw std::runtime_error("Original transform context absent");
 std::string in(s),out;if(in.size()%2)throw std::runtime_error("Original transform context hex differs");
 auto n=[](char c)->unsigned{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("Original transform hex invalid");};
 out.reserve(in.size()/2);for(size_t i=0;i<in.size();i+=2)out+=char((n(in[i])<<4)|n(in[i+1]));return out;
}
inline uint64_t N(const char* s) {
 const auto n=SharedTaskAdmission::Number(s);if(!n)throw std::runtime_error("Original transform numeric evidence differs");return *n;
}
inline auto Q(Database& db,const std::string& sql) {
 auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Original transform persistence unavailable");return r;
}
inline void Strict(Database& db) {
 auto q=Q(db,"SELECT @@in_transaction");
 if(q.RowCount()!=1||N(q.begin()[0])!=1||db.TransactionStrictFailed())throw std::runtime_error("Original transform requires writer strict transaction");
}
struct Physical {unsigned outcome=0,settled=0,abort_only=0;};
inline Physical ReadPhysical(Database& db,const std::string& key,const X::Context& c,bool lock) {
 if(!Key(key)||!X::Valid(c)||c.model!=Model)throw std::runtime_error("Original transform identity unbound");
 auto q=Q(db,fmt::format(
  "SELECT character_id,account_id,shared_task_id,phase,chest,npc_type_id,npc_entity_id,npc_spawn_id,"
  "recipe_kind,HEX(recipe_context),abort_only,physical,settled FROM kithicor_poison_custody "
  "WHERE request_key='{}'{}",key,lock?" FOR UPDATE":""));
 if(q.RowCount()!=1)throw std::runtime_error("Original transform physical identity missing");
 auto r=q.begin();
 if(N(r[0])!=c.character||N(r[1])!=c.account||N(r[2])!=(c.shared?c.occurrence:0)||N(r[3])||N(r[4])||
  N(r[5])!=c.npc_type||N(r[6])!=c.npc_entity||N(r[7])!=c.npc_spawn||N(r[8])!=3||
  Unhex(r[9])!=X::Encode(c)||N(r[10])>1||N(r[11])>3||N(r[12])>1)
  throw std::runtime_error("Original transform physical context differs");
 Physical p{unsigned(N(r[11])),unsigned(N(r[12])),unsigned(N(r[10]))};
 if((p.outcome==3)!=(p.abort_only==1))throw std::runtime_error("Original transform abort provenance differs");
 return p;
}
// This validates a NEW personal mutation, never an already-owed old return.
inline void Personal(Database& db,const X::Context& c,unsigned target,bool lock) {
 if(c.shared||c.task!=920207||target>1)throw std::runtime_error("Original personal route differs");
 auto q=Q(db,fmt::format(
  "SELECT r.occurrence_id,r.accepted_time,t.acceptedtime,t.type,t.was_rewarded,a.donecount,a.completed "
  "FROM character_task_reward_instances r JOIN character_tasks t ON t.charid=r.character_id AND t.taskid=r.task_id "
  "JOIN character_activities a ON a.charid=t.charid AND a.taskid=t.taskid AND a.activityid={} "
  "WHERE r.character_id={} AND r.task_id=920207{}",c.activity,c.character,lock?" FOR UPDATE":""));
 if(q.RowCount()!=1)throw std::runtime_error("Original personal occurrence missing");
 auto r=q.begin();
 if(N(r[0])!=c.occurrence||N(r[1])!=c.accepted_time||N(r[2])!=c.accepted_time||N(r[3])!=2||N(r[4])||N(r[5])!=target||N(r[6])!=target)
  throw std::runtime_error("Original personal occurrence/activity changed");
 auto prior=Q(db,fmt::format(
  "SELECT activityid,donecount,completed FROM character_activities WHERE charid={} AND taskid=920207 "
  "AND activityid<{} ORDER BY activityid{}",c.character,c.activity,lock?" FOR UPDATE":""));
 if(prior.RowCount()!=c.activity)throw std::runtime_error("Original personal prerequisite rows absent");
 unsigned expected=0;for(auto row:prior)if(N(row[0])!=expected++||N(row[1])!=1||N(row[2])!=1)
  throw std::runtime_error("Original personal prerequisite custody differs");
}
inline bool ActiveShared(Database& db,const X::Context& c,bool lock) {
 if(!c.shared||c.task!=920197)return false;
 auto q=Q(db,fmt::format(
  "SELECT st.id FROM shared_tasks st JOIN shared_task_members sm ON sm.shared_task_id=st.id AND sm.character_id={} "
  "WHERE st.id={} AND st.task_id=920197 AND st.accepted_time=FROM_UNIXTIME({}) "
  "AND st.completion_time IS NULL AND (st.expire_time IS NULL OR st.expire_time>NOW()){}",
  c.character,c.occurrence,c.accepted_time,lock?" FOR UPDATE":""));
 return q.RowCount()==1&&N(q.begin()[0])==c.occurrence;
}
inline X::Decision ReadFinal(Database& db,const std::string& key,const X::Context& c,bool lock) {
 auto q=Q(db,fmt::format(
  "SELECT recipe_kind,model_version,character_id,account_id,occurrence_kind,occurrence_id,task_id,activity_id,accepted_time,"
  "context_sha256=SHA2(x'{}',256),decision_kind,reject_reason,tail_before_count,tail_target_count,primary_before_count,primary_target_count,"
  "root0_accepted_units,root1_accepted_units,root2_accepted_units,root3_accepted_units "
  "FROM sod_original_delivery_decisions WHERE request_key='{}'{}",Hex(X::Encode(c)),key,lock?" FOR UPDATE":""));
 if(!q.RowCount())return {};
 if(q.RowCount()!=1)throw std::runtime_error("Original transform final decision ambiguous");
 auto r=q.begin();
 if(N(r[0])!=3||N(r[1])!=c.model||N(r[2])!=c.character||N(r[3])!=c.account||N(r[4])!=(c.shared?1u:0u)||
  (!X::Admission(c)&&(N(r[5])!=c.occurrence||N(r[8])!=c.accepted_time))||N(r[6])!=c.task||N(r[7])!=c.activity||N(r[9])!=1)
  throw std::runtime_error("Original transform final identity differs");
 X::Decision d;d.state=X::DecisionState::Known;
 if(X::Admission(c)){d.admitted_run=N(r[5]);const auto accepted=N(r[8]);if(accepted>UINT32_MAX)throw std::runtime_error("Message accepted time differs");d.admitted_accepted_time=uint32_t(accepted);}
 const auto kind=N(r[10]),reason=N(r[11]);
 d.counters_present=r[12]&&r[13]&&r[14]&&r[15];
 if(!d.counters_present&&(r[12]||r[13]||r[14]||r[15]))throw std::runtime_error("Original transform partial NULL counter tuple");
 if(d.counters_present){if(N(r[12])>4||N(r[13])>4||N(r[14])>1||N(r[15])>1)throw std::runtime_error("Original transform native counters out of bounds");d.tail_before=uint32_t(N(r[12]));d.tail_target=uint32_t(N(r[13]));d.primary_before=uint32_t(N(r[14]));d.primary_target=uint32_t(N(r[15]));}
 uint64_t units=0;for(unsigned i=0;i<4;++i){const auto n=N(r[16+i]);if(n>4)throw std::runtime_error("Original transform accepted units out of bounds");d.accepted[i]=uint32_t(n);units+=n;}
 if((kind!=1&&kind!=2)||(kind==1&&(!units||reason))||(kind==2&&(units||!reason||reason>4))||!X::Valid(c,d))
  throw std::runtime_error("Original transform final decision invalid");
 return d;
}
inline bool RunPending(Database& db,uint64_t run) {
 if(!run)throw std::runtime_error("Original transform native run absent");
 auto q=Q(db,fmt::format("SELECT COUNT(*) FROM kithicor_poison_custody WHERE recipe_kind=3 AND shared_task_id={} "
  "AND settled=0 AND abort_only=0 AND physical IN(0,1)",run));
 if(q.RowCount()!=1)throw std::runtime_error("Original transform pending outcome unavailable");
 return N(q.begin()[0])!=0;
}
inline bool Same(const X::Decision& a,const X::Decision& b) {
 return a.state==b.state&&a.accepted==b.accepted&&a.counters_present==b.counters_present&&a.tail_before==b.tail_before&&
  a.tail_target==b.tail_target&&a.primary_before==b.primary_before&&a.primary_target==b.primary_target&&
  a.admitted_run==b.admitted_run&&a.admitted_accepted_time==b.admitted_accepted_time;
}
// Caller owns the SAME strict transaction as native activity save/CAS.
inline void InsertFinal(Database& db,const std::string& key,const X::Context& c,const X::Decision& d,unsigned reject_reason=0) {
 Strict(db);if(!X::Valid(c,d)||d.state!=X::DecisionState::Known)throw std::runtime_error("Original transform decision cannot commit");
 const bool accepted=std::any_of(d.accepted.begin(),d.accepted.end(),[](uint32_t n){return n!=0;});
 if((accepted&&reject_reason)||(!accepted&&(!c.shared||reject_reason<1||reject_reason>4)))throw std::runtime_error("Original transform decision reason differs");
 const auto v=[&](uint32_t n){return d.counters_present?std::to_string(n):std::string("NULL");};
 auto q=Q(db,fmt::format(
  "INSERT INTO sod_original_delivery_decisions(request_key,recipe_kind,model_version,character_id,account_id,occurrence_kind,occurrence_id,task_id,activity_id,"
  "accepted_time,context_sha256,decision_kind,reject_reason,tail_before_count,tail_target_count,primary_before_count,primary_target_count,"
  "root0_accepted_units,root1_accepted_units,root2_accepted_units,root3_accepted_units,created_at) "
  "VALUES('{}',3,{},{},{},{},{},{},{},{},SHA2(x'{}',256),{},{},{},{},{},{},{},{},{},{},UNIX_TIMESTAMP())",
  key,c.model,c.character,c.account,c.shared?1:0,X::Admission(c)?d.admitted_run:c.occurrence,c.task,c.activity,X::Admission(c)?d.admitted_accepted_time:c.accepted_time,Hex(X::Encode(c)),accepted?1:2,reject_reason,
  v(d.tail_before),v(d.tail_target),v(d.primary_before),v(d.primary_target),d.accepted[0],d.accepted[1],d.accepted[2],d.accepted[3]));
 if(q.RowsAffected()!=1)throw std::runtime_error("Original transform final decision was not inserted");
}
}

namespace OriginalTransformNative {
inline X::Decision ReadDecision(Database& db,const std::string& key,const X::Context& c) {
 SodOriginalTransform::ReadPhysical(db,key,c,true);
 return SodOriginalTransform::ReadFinal(db,key,c,true);
}
inline bool PrepareIntent(Database& db,const std::string& key,const X::Context& c) {
 try{
  SodOriginalTransform::Strict(db);const auto p=SodOriginalTransform::ReadPhysical(db,key,c,true);
  if(p.outcome||p.settled||p.abort_only||SodOriginalTransform::ReadFinal(db,key,c,true).state!=X::DecisionState::Pending)return false;
  if(X::Admission(c))return true; // Preadmission run/time remain zero; no synthetic active task.
  if(c.shared)return SodOriginalTransform::ActiveShared(db,c,false);
  SodOriginalTransform::Personal(db,c,0,true);return true;
 }catch(...){return false;}
}
inline bool CommitSettlement(Database& db,const std::string& key,const X::Context& c,const X::Decision& d) {
 try{
  SodOriginalTransform::Strict(db);const auto p=SodOriginalTransform::ReadPhysical(db,key,c,true);
  if(p.abort_only||p.settled||p.outcome!=0)return false;
  const auto prior=SodOriginalTransform::ReadFinal(db,key,c,true);
  if(c.shared)return d.state==X::DecisionState::Known&&SodOriginalTransform::Same(prior,d);
  if(prior.state!=X::DecisionState::Pending||d.state!=X::DecisionState::PreparedPersonal||!X::Valid(c,d))return false;
  SodOriginalTransform::Personal(db,c,0,true);
  auto q=SodOriginalTransform::Q(db,fmt::format("UPDATE character_activities SET donecount=1,completed=1 "
   "WHERE charid={} AND taskid=920207 AND activityid={} AND donecount=0 AND completed=0",c.character,c.activity));
  if(q.RowsAffected()!=1)return false;
  SodOriginalTransform::Personal(db,c,1,true);
  auto committed=d;committed.state=X::DecisionState::Known;
  SodOriginalTransform::InsertFinal(db,key,c,committed);return true;
 }catch(...){return false;}
}
inline bool MarkReturned(Database& db,const std::string& key,const X::Context& c,unsigned original_physical) {
 try{
  SodOriginalTransform::Strict(db);const auto p=SodOriginalTransform::ReadPhysical(db,key,c,true);
  if((original_physical!=0&&original_physical!=3)||p.outcome!=original_physical||p.settled)return false;
  const auto d=SodOriginalTransform::ReadFinal(db,key,c,true);
  if(X::Admission(c)) {
   // Physical3/abort_only proves the intent never reached Prepared selector
   // authority. No submitted World admission can exist for this abort image.
   if(p.abort_only&&original_physical==3&&d.state==X::DecisionState::Pending)return true;
   return d.state==X::DecisionState::Known&&std::all_of(d.accepted.begin(),d.accepted.end(),[](uint32_t n){return n==0;});
  }
  if(p.abort_only)return original_physical==3&&d.state==X::DecisionState::Pending;
  if(!c.shared)return d.state==X::DecisionState::Pending; // no native mutation; no active-task join for owed return
  return d.state==X::DecisionState::Known&&std::all_of(d.accepted.begin(),d.accepted.end(),[](uint32_t n){return n==0;});
 }catch(...){return false;}
}
inline bool ValidateCommitted(Database& db,const std::string& key,const X::Context& c) {
 try{
  const auto p=SodOriginalTransform::ReadPhysical(db,key,c,true);
  return p.outcome==1&&!p.abort_only&&SodOriginalTransform::ReadFinal(db,key,c,true).state==X::DecisionState::Known;
 }catch(...){return false;}
}
}

#include "sod_message_admission.h"
