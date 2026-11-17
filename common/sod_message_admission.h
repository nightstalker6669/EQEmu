#pragma once
#include "sod_original_transform_native.h"
#include "sod_admission.h"
namespace EQEmu::SharedCore::OriginalTradeCustody {
void ValidateWorldOutboxUnderTransaction(Database&,const std::string&,const OriginalTradeTransform::Context&);
}
#include "shared_core/original_message_admission_native.h"
#include <cstring>

// One bounded native selector attempt for an existing original physical key.
// No inventory blobs or alternate task/reward occurrence allocator live here.
namespace SodMessageAdmission {
static_assert(sizeof(SodAdmission::Proof)==120,"Message schema native proof size differs");
namespace S=SodOriginalTransform;
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
inline bool Task(uint32_t id){return id>=920200&&id<=920203;}
inline std::string ProofBytes(const SodAdmission::Proof& p){return std::string(reinterpret_cast<const char*>(&p),sizeof(p));}
struct Attempt {std::string key;X::Context context;uint32_t state=0;uint64_t nonce=0;};
inline void LockActorPhysical(Database& db,const std::string& key,const X::Context& c) {
 S::Strict(db);if(!S::Key(key)||!X::Admission(c)||!X::Valid(c))throw std::runtime_error("Message context differs");
 auto actor=S::Q(db,fmt::format("SELECT account_id FROM housing_inventory_sessions WHERE character_id={} FOR UPDATE",c.character));
 if(actor.RowCount()!=1||S::N(actor.begin()[0])!=c.account)throw std::runtime_error("Message actor lease absent");
 S::ReadPhysical(db,key,c,true); // Actor -> physical -> selector/native run.
}
inline Attempt Read(Database& db,const std::string& key,const X::Context& c,bool may_be_absent=false) {
 auto q=S::Q(db,fmt::format("SELECT character_id,account_id,task_id,npc_type_id,npc_entity_id,npc_spawn_id,selector_generation,state,native_nonce,context_sha256=SHA2(x'{}',256) FROM sod_original_message_selectors WHERE request_key='{}' FOR UPDATE",S::Hex(X::Encode(c)),key));
 if(!q.RowCount()&&may_be_absent)return Attempt{key,c,UINT32_MAX,0};
 if(q.RowCount()!=1)throw std::runtime_error("Message selector unavailable");auto r=q.begin();
 if(S::N(r[0])!=c.character||S::N(r[1])!=c.account||S::N(r[2])!=c.task||S::N(r[3])!=c.npc_type||S::N(r[4])!=c.npc_entity||S::N(r[5])!=c.npc_spawn||S::N(r[6])!=1||S::N(r[7])>2||S::N(r[9])!=1)throw std::runtime_error("Message selector identity differs");
 const auto state=uint32_t(S::N(r[7]));const auto nonce=r[8]?S::N(r[8]):0;
 if((state==1)!=bool(nonce))throw std::runtime_error("Message selector nonce differs");
 return Attempt{key,c,state,nonce};
}
inline void RejectLocked(Database& db,const std::string& key,const X::Context& c,bool allow_missing) {
 auto a=Read(db,key,c,allow_missing);
 const auto prior=S::ReadFinal(db,key,c,true);
 if(prior.state==X::DecisionState::Known) {
  if(std::any_of(prior.accepted.begin(),prior.accepted.end(),[](auto n){return n!=0;}))throw std::runtime_error("Accepted message cannot be returned");
  return;
 }
 if(a.state==1)throw std::runtime_error("Submitted World message outcome is still pending");
 if(a.state==0) {
  auto q=S::Q(db,fmt::format("UPDATE sod_original_message_selectors SET state=2,updated_at=UNIX_TIMESTAMP() WHERE request_key='{}' AND state=0 AND native_nonce IS NULL",key));
  if(q.RowsAffected()!=1)throw std::runtime_error("Message decline raced native submission");
 }
 X::Decision d;d.state=X::DecisionState::Known;S::InsertFinal(db,key,c,d,4);
}
inline std::optional<Attempt> ForWorld(Database& db,const SodAdmission::Proof& p,uint32_t entity) {
 S::Strict(db);if(!Task(p.task)||!SodAdmission::Shape(p))return std::nullopt;
 // Nonlocking identity discovery only. Re-read all authority after canonical
 // actor/physical locks, then lock the selector. No physical row locked first.
 auto find=S::Q(db,fmt::format("SELECT s.request_key,HEX(j.recipe_context) FROM sod_original_message_selectors s JOIN kithicor_poison_custody j ON j.request_key=s.request_key WHERE s.character_id={} AND s.task_id={} AND s.native_nonce={} AND s.state=1 AND j.recipe_kind=3",p.requester,p.task,p.nonce));
 if(!find.RowCount())return std::nullopt;if(find.RowCount()!=1||!find.begin()[0])throw std::runtime_error("Message nonce ambiguous");
 const std::string key=find.begin()[0];const auto c=X::Decode(S::Unhex(find.begin()[1]));
 if(c.character!=p.requester||c.task!=p.task||c.npc_entity!=entity||p.zone!=452||p.instance)return std::nullopt;
 LockActorPhysical(db,key,c);auto a=Read(db,key,c);
 auto exact=S::Q(db,fmt::format("SELECT native_roster_proof=x'{}' FROM sod_original_message_selectors WHERE request_key='{}' AND state=1 AND native_nonce={} FOR UPDATE",S::Hex(ProofBytes(p)),key,p.nonce));
 if(a.nonce!=p.nonce||exact.RowCount()!=1||S::N(exact.begin()[0])!=1)throw std::runtime_error("Message native roster proof differs");
 return a;
}
inline void AcceptLocked(Database& db,const Attempt& a,uint64_t run,uint32_t accepted) {
 S::Strict(db);if(!run||!accepted||a.state!=1)throw std::runtime_error("Message canonical admission absent");
 if(S::ReadFinal(db,a.key,a.context,true).state!=X::DecisionState::Pending)throw std::runtime_error("Message already decided");
 X::Decision d;d.state=X::DecisionState::Known;d.accepted[a.context.primary]=1;d.admitted_run=run;d.admitted_accepted_time=accepted;
 S::InsertFinal(db,a.key,a.context,d);
}
inline void RefuseSubmittedLocked(Database& db,const Attempt& a,unsigned reason=4) {
 S::Strict(db);if(a.state!=1)throw std::runtime_error("Message submission absent");
 const auto prior=S::ReadFinal(db,a.key,a.context,true);if(prior.state==X::DecisionState::Known)return;
 X::Decision d;d.state=X::DecisionState::Known;S::InsertFinal(db,a.key,a.context,d,reason);
}
}

namespace OriginalMessageAdmissionNative {
inline void FinalizeUnsubmittedOnOwnerDeath(Database& db,const std::string& key,const X::Context& c) {
 // Caller has actual complete-delete/supervisor authority and the same
 // original full-return transaction. Absence proves no BindNonce could send.
 SodMessageAdmission::LockActorPhysical(db,key,c);
 // A known accepted World receipt is consumed by the SAME common recovery
 // path; it must not be mistaken for a request to refund that accepted note.
 if(SodOriginalTransform::ReadFinal(db,key,c,true).state==X::DecisionState::Known)return;
 SodMessageAdmission::RejectLocked(db,key,c,true);
}
}
