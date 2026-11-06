#pragma once
#include "fire_run_action_custody.h"
#include "common/item_instance.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <glm/glm.hpp>
class Doors;
class NPC;
class Corpse;
class EntityList;
class MoveToCommand;class FlyToCommand;class SwimToCommand;
class Zone;
namespace FireSharedTransformSource {class PublishedSharedTransformSink;struct PublishedSharedTransformView;}
namespace FireRunActionSource {
class RunUseWitness;
class RunHelperAttempt;
class RunDoorLifetime final {friend class ::Doors;friend class RunUseWitness;bool active_=true;RunDoorLifetime()=default;public:~RunDoorLifetime()=default;};
class RunCorpseLifetime final {friend class ::Corpse;friend class RunUseWitness;bool active_=true;RunCorpseLifetime()=default;public:~RunCorpseLifetime()=default;};
enum class GateRole:uint8_t {InnerVault=1,WorshipLeft=2,WorshipRight=3,KeymasterGarden=4,KeymasterPool=5,GuardianVault=6,WorldFont=7};
// Read-only facts are not authority. Only original private native issuers can
// construct the owning uses below, with their actual lifetime/click witnesses.
struct MissionRunFacts {
 uint64_t run=0,local_occurrence=0;
 uint32_t character=0,accepted_time=0,zone=0,version=0,instance=0,dynamic_zone=0;
};
struct PhysicalItemFacts {
 uint32_t item_id=0;
 int32_t serial=0;
 int16_t slot=0,charges=0;
 uint16_t units=0;
};
struct DoorUseFacts {
 RunActionKind kind{};
 GateRole role{};
 std::optional<MissionRunFacts> mission; // absent ONLY for a real world Font click
 uint64_t native_sequence=0;
 uint32_t character=0,client_entity=0,zone=0,version=0,instance=0,door_database_id=0;
 uint8_t native_door_id=0;
 std::array<uint32_t,3> required_item_ids{};
 uint8_t required_count=0;
 std::array<PhysicalItemFacts,3> physical_items{};
 uint8_t physical_count=0;
 // Material facts may be absent for a later published-latch leaf. A first
 // action needs exact captured originals; Core never manufactures possession.
};
class NativeRunHelperSource;
class OriginalDoorUse final {
 friend class NativeRunDoorSource;
 friend class PreparedRunDoorPlan;
 friend class PublishedRunActionSink;
 struct State;
 DoorUseFacts facts_;
 std::shared_ptr<State> state_;
 OriginalDoorUse(DoorUseFacts,std::shared_ptr<State>);
public:
 ~OriginalDoorUse();
 const DoorUseFacts& Facts()const noexcept{return facts_;}
 OriginalDoorUse(const OriginalDoorUse&)=delete;
 OriginalDoorUse& operator=(const OriginalDoorUse&)=delete;
};
struct HelperUseFacts {
 MissionRunFacts mission;
 PhysicalItemFacts clicked; // actual47756/charge1 original slot/serial
 uint32_t caster_entity=0,target_entity=0;
 GateRole selected_target{}; // actual native location chooses garden OR pool
};
class OriginalHelperUse final {
 friend class NativeRunHelperSource;
 friend class PreparedRunHelperPlan;
 friend class PublishedRunActionSink;
 struct State;
 HelperUseFacts facts_;
 std::shared_ptr<State> state_;
 OriginalHelperUse(HelperUseFacts,std::shared_ptr<State>);
public:
 ~OriginalHelperUse();
 const HelperUseFacts& Facts()const noexcept{return facts_;}
 OriginalHelperUse(const OriginalHelperUse&)=delete;
 OriginalHelperUse& operator=(const OriginalHelperUse&)=delete;
};
class PreparedRunDoorPlan final {
 friend class NativeRunDoorSource;
 friend class NativeRunActionCustody;
 friend class PublishedRunActionSink;
 struct State;
 std::unique_ptr<State> state_;
 explicit PreparedRunDoorPlan(std::unique_ptr<State>)noexcept;
 static std::unique_ptr<PreparedRunDoorPlan> PrepareActual(Client&,const OriginalDoorUse&);
 bool RevalidateBeforePersist(Client&)const;
public:
 ~PreparedRunDoorPlan();
 PreparedRunDoorPlan(const PreparedRunDoorPlan&)=delete;
 PreparedRunDoorPlan& operator=(const PreparedRunDoorPlan&)=delete;
};
class PreparedRunHelperPlan final {
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 friend class NativeRunHelperSource;
 friend class PublishedRunActionSink;
 struct State;
 std::unique_ptr<State> state_;
 explicit PreparedRunHelperPlan(std::unique_ptr<State>)noexcept;
 static std::unique_ptr<PreparedRunHelperPlan> PrepareActual(Client&,const OriginalHelperUse&,uint64_t);
 bool RevalidateBeforePersist(Client&)const;
public:
 ~PreparedRunHelperPlan();
 PreparedRunHelperPlan(const PreparedRunHelperPlan&)=delete;
 PreparedRunHelperPlan& operator=(const PreparedRunHelperPlan&)=delete;
};
// Positive native publication is a different private event from a Core commit.
// Only genuine original door or actual owned helper lifecycle can construct it.
struct NativeRunPublicationFacts {
 RunActionKind kind{};
 GateRole role{};
 uint64_t run_action_id=0,run_id=0,native_sequence=0;
 uint32_t character=0,zone=0,instance=0,door_database_id=0,actor_entity=0;
 uint8_t native_door_id=0;
};
class NativeRunPublication final {
 friend class RunHelperAttempt;
 friend class PreparedRunDoorPlan;
 friend class PreparedRunHelperPlan;
 friend class PublishedRunActionSink;
 friend class NativeRunDoorSource;
 friend class NativeRunHelperSource;
 friend class NativeRunActionCustody;
 NativeRunPublicationFacts facts_;
 bool taken_=false; // Core seals before native publication-ack SQL
 struct State;
 std::unique_ptr<State> state_;
 explicit NativeRunPublication(std::unique_ptr<State>)noexcept;
public:
 ~NativeRunPublication();
 NativeRunPublication(const NativeRunPublication&)=delete;
 NativeRunPublication& operator=(const NativeRunPublication&)=delete;
};
class NativeRunDoorSource final {
 friend class ::Client;
 friend class ::Doors;
 friend class PublishedRunActionSink;
 friend class NativeRunHelperSource;
 friend class PreparedRunDoorPlan;
 static bool RevalidateOriginalUse(Client&,const OriginalDoorUse&);
 static bool PublishHelperBarrier(RunHelperAttempt&);
 // Actual original packet before ordinary mutable callbacks. True means the
 // selected branch handled/rejected and native caller MUST return immediately.
 static bool HandleOriginalPacket(Client&,Doors&,uint8_t actual_packet_door);
 static bool AuthorizeActualHandle(Client*,Doors&,uint8_t actual_trigger);
 static void ObserveActualOpened(Doors&);
};
class NativeRunHelperSource final {
 friend class ::MoveToCommand;friend class ::FlyToCommand;friend class ::SwimToCommand;
 static void RecordActualWalkingStep(NPC&,const glm::vec3&,const glm::vec3&,int actual_native_mode);
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 friend class ::NPC;
 friend class ::EntityList;
 friend class ::Zone;
 friend class PreparedRunHelperPlan;
 friend class PublishedRunActionSink;
 static void PublishActual(Client&,const PublishedRunActionView&);
 static void ObserveActualRegistration(NPC*);
 static void ObserveActualDestruction(NPC&);
 static void DrainCompletions();
 static void DiscardCompletions()noexcept;
 static std::unique_ptr<OriginalHelperUse> CaptureActual(Client&,const EQ::ItemInstance&,
  int16_t actual_slot,bool native_slot_supported,uint32_t actual_target);
 static bool RevalidateActual(Client&,const OriginalHelperUse&);
 // Actual private owned actor's native Process. Empty preserves ordinary NPCs.
 static std::optional<bool> ProcessActual(NPC&);
public:
 // Actual native movement fallback only; rejects teleport for the genuinely
 // owned helper and terminally holds that attempt. Never grants progress.
 static bool RejectActualTeleport(NPC&);
};
class PublishedRunActionSink final {
public:
 static void Receive(Client&,PublishedRunActionReceipt&);
};
} // namespace FireRunActionSource
