#pragma once
#include "original_trade_transform.h"
#include <string>
class Database;
namespace OriginalMessageAdmissionNative {
namespace X=EQEmu::SharedCore::OriginalTradeTransform;
// Spells semantic owner. Actual complete-delete/supervisor proof ONLY, inside
// the SAME strict full-return transaction/lease. A durable unsubmitted selector
// may be rejected; an already submitted native nonce remains held until actual
// canonical World admission/rejection is known. No missing-outcome inference.
void FinalizeUnsubmittedOnOwnerDeath(Database&,const std::string& request_key,const X::Context&);
}

// Common inline implementation is required in Zone and supervisor recovery.
#include "../sod_message_admission.h"
