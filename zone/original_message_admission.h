#pragma once
#include "original_trade_transform_hooks.h"
class Client;class NPC;
namespace KithicorPoisonTrade {class Writer;}
namespace OriginalMessageAdmission {
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
struct PendingSelection {std::string request_key;X::Context context;};
// Native opcode/LastOffers owner only; not Lua. This retrieves the real held
// offer, not possession. Caller must still issue its genuine positive-choice
// scope and bind the existing SodAdmission nonce durably before World send.
std::optional<PendingSelection> FindForNativeSelection(Client&,uint32_t task,uint32_t npc_entity);
// Only the SAME original writer can open the selector from a committed held
// input. This is not possession-only authority or a Lua-callable handin count.
class Prepared final {
 friend class KithicorPoisonTrade::Writer;
 const std::string& key_;const X::Context& context_;
 Prepared(const std::string& k,const X::Context& c):key_(k),context_(c){}
public:
 Prepared(const Prepared&)=delete;Prepared& operator=(const Prepared&)=delete;
 const std::string& Key()const{return key_;}const X::Context& Context()const{return context_;}
};
// Spells semantic/selector owner. Open exact one-task server-owned native offer
// after known physical intent. No debit, admission, activity or reward on open.
void OpenNativeSelector(Client&,NPC&,const Prepared&);
// Called only for validated OP_CancelTrade on this same original live trade.
// Native CAS declines an offered/unsubmitted selector; submitted/unknown World
// outcomes cannot be cancelled/refunded speculatively. It inserts a known final
// rejection in its own strict transaction, after which Core resumes same key.
bool DeclineUnsubmitted(Client&,const std::string& key,const X::Context&);
}
namespace OriginalTradeTransforms {
// Core guard for actual original trade cancellation, not selector task_id0 or
// window closing. No synthetic decline from silence/timeouts is defined.
void DeclineOriginalMessageTrade(Client&)noexcept;
}
