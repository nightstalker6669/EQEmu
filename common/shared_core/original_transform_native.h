#pragma once
#include "original_trade_transform.h"
#include "inventory_session_service.h"
#include <optional>
class Database;
// Spells native semantic owner. Same physical row is the durable request/outbox;
// a final decision row is semantic only, never another physical property blob.
namespace OriginalTransformNative {
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
using Lease=EQEmu::SharedCore::InventorySessions::Lease;
// These common helpers must link in both Zone and supervisor recovery.
// ReadDecision locks exact final decision and byte-exact context; Pending is
// never interpreted as rejection. No current-active-task join for owed returns.
X::Decision ReadDecision(Database&,const std::string& key,const X::Context&);
// Prepare is inside the same physical intent transaction. No activity grant.
bool PrepareIntent(Database&,const std::string& key,const X::Context&);
// Personal CAS/native final decision shares physical output transaction. Shared
// path confirms existing World final decision, never reissues activity delta.
bool CommitSettlement(Database&,const std::string& key,const X::Context&,const X::Decision&);
// Only after verified full original return for personal intent or abort-only3.
// Shared physical0 must first have a known World decision; no guessed cancel.
bool MarkReturned(Database&,const std::string& key,const X::Context&,unsigned original_physical);
// Validate physical1 and final native decision when recovering known output.
bool ValidateCommitted(Database&,const std::string& key,const X::Context&);
}

// Exact Spells common implementation: reaches Zone AND supervisor recovery.
#include "../sod_original_transform_native.h"
