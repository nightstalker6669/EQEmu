#pragma once
#include <cstdint>
class Client;
class NPC;
namespace FirePersonalOccurrenceSource {
enum class InitialAdmissionResult { Hold,SubmittedNotAccepted,AcceptedBeforeCallbacks };
// Finite native Gotonur initial-request entry; no scripting getter or fake
// preceding reward/claim. Actual acceptance evidence stays inside task cpp.
class InitialAdmissionNative final {
public:
 static InitialAdmissionResult Request(Client&,NPC&,uint32_t actual_zone,
  uint32_t actual_instance,uint32_t selected_original_giver_spawn);
};
} // namespace FirePersonalOccurrenceSource
