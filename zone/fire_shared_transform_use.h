#pragma once
#include "common/item_instance.h"
#include <array>
#include <cstdint>
#include <optional>
class Client;
class ClientTaskState;
namespace FirePersonalOccurrenceSource {class NativeItemCustody;}
namespace FireSharedTransformSource {
inline constexpr uint32_t Task=920170;
struct SharedTransformRule {
 uint32_t clicked_item=0,output_item=0;
 int activity=0;
 std::array<uint32_t,5> inputs{}; // one physical unit of each, ONE actual holder
 uint8_t input_count=0;
};
inline std::optional<SharedTransformRule> OriginalRule(uint32_t clicked) noexcept {
 if(clicked==47760)return SharedTransformRule{47760,47762,6,{{47760,47761,0,0,0}},2};
 if(clicked==47752)return SharedTransformRule{47752,47756,13,{{47751,47752,47753,47754,47755}},5};
 return {};
}
struct SharedTransformContext {
 uint64_t run=0,local_occurrence=0;
 uint32_t character=0,accepted_time=0,zone_id=0,instance_id=0,dynamic_zone_id=0;
 SharedTransformRule rule{};
};
struct OriginalSharedTransformUse {
 SharedTransformContext shared;
 uint32_t item_id=0,caster_entity=0,target_entity=0;
 int32_t item_serial=0;
 int16_t slot=0,charges_before=0;
};
class PublishedSharedTransformUse;
class PublishedSharedTransformSink;
class NativeSharedTransform final {
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 friend class PublishedSharedTransformSink;
 friend class ::ClientTaskState;
 struct Dispatch {const OriginalSharedTransformUse* original;Client* owner;bool entered=false;};
 static thread_local Dispatch* dispatch_;
 static bool ClaimActualSubmission(Client&,uint32_t task,int activity,uint64_t actual_run);
 static std::optional<SharedTransformContext> CaptureContextActual(Client&,uint32_t clicked_item);
 static bool ContextMatchesActual(Client&,const SharedTransformContext&);
 // Actual original supported-slot/click invocation only; no event from history.
 static std::optional<OriginalSharedTransformUse> CaptureActual(Client&,const EQ::ItemInstance&,
  int16_t actual_slot,bool native_slot_supported,uint32_t actual_target);
 static bool RevalidateActual(Client&,const OriginalSharedTransformUse&);
};
class PublishedSharedTransformSink final {
public:
 static void Receive(Client&,PublishedSharedTransformUse&);
};
// Core owns the opaque private issuer/Take receipt and SAME post-COMMIT callsite.
void OnOriginalSharedTransformPublished(Client&,PublishedSharedTransformUse&);
} // namespace FireSharedTransformSource
