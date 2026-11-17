#pragma once
#include "../common/shared_core/original_transform_native.h"
#include <optional>
class Client;class NPC;class ZoneDatabase;
namespace KithicorPoisonTrade {class Writer;}
namespace OriginalTradeTransforms {
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
class Committed final {
 friend class KithicorPoisonTrade::Writer;
 const std::string& key_;const X::Context& context_;
 Committed(const std::string& k,const X::Context& c):key_(k),context_(c){}
public:
 Committed(const Committed&)=delete;Committed& operator=(const Committed&)=delete;
 const std::string& Key()const{return key_;}const X::Context& Context()const{return context_;}
};
// Spells owns exact current native occurrence/prerequisite attribution. Source
// bindings supplied by PHYSICAL-TRANSFORM-CONTRACT; Calethos remains refused.
bool ClaimsOffer(Client&,NPC&);
std::optional<X::Context> SnapshotOriginal(Client&,NPC&,const OriginalTransformNative::Lease&,ZoneDatabase&);
// Called after known physical intent commit. Same request only; packet is a hint,
// World must recover durable kind3/physical0 shared outbox rows after loss/restart.
void SubmitSharedSameRequest(const std::string& key,const X::Context&);
// Known physical publication only. Never increment original delta twice and
// never project an old personal/shared occurrence onto a new task acceptance.
bool Acknowledge(Client&,const Committed&);
// Core entrance/continuation called only by actual original accept / native ack.
bool OriginalAccept(Client&,NPC&);
bool Pending(Client&);
void PollPending(Client&)noexcept;
void ResumeKnownDecision(Client&,const std::string& key);
void AfterCompleteDelete(uint32_t character,uint64_t epoch)noexcept;
}
