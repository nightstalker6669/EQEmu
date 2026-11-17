#include "client.h"
#include "npc.h"
#include "zone.h"
#include "zonedb.h"
#include "task_manager.h"
#include "sod_message_admission.h"
#include "common/sod_message_admission.h"
#include "common/shared_core/original_trade_custody.h"

namespace SodMessageAdmission {
namespace {thread_local NativeChoice* choice=nullptr;}
NativeChoice::NativeChoice(Client& c,uint32_t task,uint32_t entity):actor_(&c),task_(task),entity_(entity),previous_(choice) {
 if(Task(task))pending_=OriginalMessageAdmission::FindForNativeSelection(c,task,entity);
 choice=this;
}
NativeChoice::~NativeChoice(){choice=previous_;}
bool BindNonce(Client& c,const SodAdmission::Proof& proof) {
 if(!Task(proof.task))return true;
 if(!choice||choice->actor_!=&c||choice->task_!=proof.task||!choice->pending_||
  !SodAdmission::Shape(proof)||proof.requester!=c.CharacterID()||proof.zone!=452||proof.instance)return false;
 const auto& pending=*choice->pending_;const auto& x=pending.context;
 if(x.npc_entity!=choice->entity_||x.character!=c.CharacterID()||x.account!=c.AccountID())return false;
 if(!database.TransactionBeginStrict().Success())return false;
 try {
  LockActorPhysical(database,pending.request_key,x);
  EQEmu::SharedCore::OriginalTradeCustody::ValidateWorldOutboxUnderTransaction(database,pending.request_key,x);
  auto a=Read(database,pending.request_key,x);
  if(a.state!=0||SodOriginalTransform::ReadFinal(database,a.key,x,true).state!=X::DecisionState::Pending)throw std::runtime_error("Message selector is no longer offered");
  auto q=S::Q(database,fmt::format("UPDATE sod_original_message_selectors SET state=1,native_nonce={},native_roster_proof=x'{}',updated_at=UNIX_TIMESTAMP() WHERE request_key='{}' AND state=0 AND native_nonce IS NULL",proof.nonce,S::Hex(ProofBytes(proof)),a.key));
  if(q.RowsAffected()!=1)throw std::runtime_error("Message native choice raced decline");
  if(!database.TransactionCommitStrict().Success())throw std::runtime_error("Message submission commit unknown");
  return true;
 }catch(...){database.TransactionRollbackStrict();return false;}
}
}
namespace OriginalMessageAdmission {
void OpenNativeSelector(Client& c,NPC& npc,const Prepared& proof) {
 namespace M=SodMessageAdmission;const auto& x=proof.Context();const auto& key=proof.Key();
 if(!zone||zone->GetZoneID()!=452||zone->GetInstanceID()||zone->GetInstanceVersion()||
  c.CharacterID()!=x.character||c.AccountID()!=x.account||npc.GetID()!=x.npc_entity||npc.GetNPCTypeID()!=x.npc_type||npc.GetSpawnPointID()!=x.npc_spawn)return;
 if(!database.TransactionBeginStrict().Success())return;
 bool open=false;
 try {
  M::LockActorPhysical(database,key,x);
  auto prior=M::Read(database,key,x,true);
  if(prior.state!=UINT32_MAX)throw std::runtime_error("Message selector attempt already exists");
  if(M::S::ReadFinal(database,key,x,true).state!=X::DecisionState::Pending)throw std::runtime_error("Message already decided");
  auto q=M::S::Q(database,fmt::format("INSERT INTO sod_original_message_selectors(request_key,character_id,account_id,task_id,npc_type_id,npc_entity_id,npc_spawn_id,selector_generation,state,native_nonce,native_roster_proof,context_sha256,created_at,updated_at) VALUES('{}',{},{},{},{},{},{},1,0,NULL,NULL,SHA2(x'{}',256),UNIX_TIMESTAMP(),UNIX_TIMESTAMP())",key,x.character,x.account,x.task,x.npc_type,x.npc_entity,x.npc_spawn,M::S::Hex(X::Encode(x))));
  if(q.RowsAffected()!=1||!database.TransactionCommitStrict().Success())throw std::runtime_error("Message selector preparation unknown");
  open=true;
 }catch(...){database.TransactionRollbackStrict();}
 if(open)TaskManager::Instance()->SharedTaskSelector(&c,&npc,{int(x.task)},false);
}
bool DeclineUnsubmitted(Client& c,const std::string& key,const X::Context& x) {
 if(c.CharacterID()!=x.character||c.AccountID()!=x.account)return false;
 if(!database.TransactionBeginStrict().Success())return false;
 try {
  SodMessageAdmission::LockActorPhysical(database,key,x);
  SodMessageAdmission::RejectLocked(database,key,x,false);
  if(!database.TransactionCommitStrict().Success())throw std::runtime_error("Message decline commit unknown");
  return true;
 }catch(...){database.TransactionRollbackStrict();return false;}
}
}
