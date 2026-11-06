#pragma once
#include "fire_personal_item_use.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>
class Client;
class NPC;
struct NPCType;
namespace FirePersonalOccurrenceSource {
class NativeItemCustody;
class PublishedNativeUseSink;
class OriginalConstructAttempt;
struct ActivationParticipant {
 Binding original; // eligibility snapshot only; never physical/kill proof
};
class PreparedConstructPlan final {
 friend class NativeItemCustody;
 friend class PublishedNativeUseSink;
 OriginalItemUse original_;
 uint64_t sequence_;
 glm::vec4 point_{};
 float native_ground_=0;
 uint32_t group_id_=0;
 std::unique_ptr<NPCType> expected_definition_;
 std::shared_ptr<OriginalConstructAttempt> attempt_;
 std::unique_ptr<NPC> actor_;
 PreparedConstructPlan(const OriginalItemUse& original,uint64_t sequence) noexcept;
 // Called ONLY from genuine selected ordinary-debit entry, before live spend.
 static std::unique_ptr<PreparedConstructPlan> PrepareActual(Client&,const OriginalItemUse&,uint64_t);
 bool RevalidateBeforePersist(Client&) const;
public:
 ~PreparedConstructPlan(); // actual NPC and owned type destroyed only where complete
 PreparedConstructPlan(const PreparedConstructPlan&)=delete;
 PreparedConstructPlan& operator=(const PreparedConstructPlan&)=delete;
 PreparedConstructPlan(PreparedConstructPlan&&)=delete;
 PreparedConstructPlan& operator=(PreparedConstructPlan&&)=delete;
};
} // namespace FirePersonalOccurrenceSource
