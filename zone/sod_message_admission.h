#pragma once
#include "original_message_admission.h"
#include "common/sod_admission.h"
class Client;
namespace SodMessageAdmission {
class NativeChoice final {
 friend class ::Client;
 friend bool BindNonce(Client&,const SodAdmission::Proof&);
 Client* actor_;uint32_t task_,entity_;
 std::optional<OriginalMessageAdmission::PendingSelection> pending_;
 NativeChoice* previous_;
 NativeChoice(Client&,uint32_t task,uint32_t entity);
public:
 ~NativeChoice();
 NativeChoice(const NativeChoice&)=delete;NativeChoice& operator=(const NativeChoice&)=delete;
};
bool BindNonce(Client&,const SodAdmission::Proof&);
}
