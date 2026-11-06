#pragma once
#include "fire_original_construct_producer.h"
namespace FirePersonalOccurrenceSource {
// Called only at the existing targeted, visible, in-range, unengaged NPC say path.
class OriginalConstructNativeHail final {
public:
 static void OnActualTargetedSay(Client&,NPC&,const char* actual_message);
};
} // namespace FirePersonalOccurrenceSource
