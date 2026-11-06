#pragma once
#include "fire_run_action_scene.h"
#include <vector>
class TaskManager;
namespace FireRunActionSource {
class NativeRunTerminalCustody;
class NativeRunTerminalSource;
class OriginalTerminalDeath;
class RunTerminalAttempt;
class RunUseWitness;
enum class OriginalTerminalReturnResult {RejectedBeforeMutation,KnownCommitted,UnknownHold};
struct TerminalAcceptedMemberFacts {
 uint32_t character=0,accepted_time=0;uint64_t local_occurrence=0;
 bool operator==(const TerminalAcceptedMemberFacts& b)const noexcept {
  return character==b.character&&accepted_time==b.accepted_time&&local_occurrence==b.local_occurrence;
 }
};
struct TerminalReturnFacts {
 MissionRunFacts mission;
 PhysicalItemFacts mold;
 uint64_t native_sequence=0;
 uint32_t client_entity=0,giver_entity=0,giver_spawn=0,giver_npc=0;
 uint64_t instance_expire_at=0;
 std::vector<TerminalAcceptedMemberFacts> accepted_members;
};
// A read-only snapshot is not authority. This opaque use can be minted only
// by the actual selected native NPC trade before its first generic removal.
class OriginalMoldTrade final {
 friend class NativeRunTerminalSource;
 friend class PreparedTerminalChestPlan;
 friend class PublishedTerminalReturnSink;
 struct State;
 std::shared_ptr<State> state_;
 TerminalReturnFacts facts_;
 OriginalMoldTrade(TerminalReturnFacts,std::shared_ptr<State>);
public:
 ~OriginalMoldTrade();
 const TerminalReturnFacts& Facts()const noexcept{return facts_;}
 OriginalMoldTrade(const OriginalMoldTrade&)=delete;
 OriginalMoldTrade& operator=(const OriginalMoldTrade&)=delete;
};
class PreparedTerminalChestPlan final {
 friend class NativeRunTerminalSource;
 friend class NativeRunTerminalCustody;
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 friend class PublishedTerminalReturnSink;
 struct State;
 std::unique_ptr<State> state_;
 explicit PreparedTerminalChestPlan(std::unique_ptr<State>)noexcept;
 static std::unique_ptr<PreparedTerminalChestPlan> PrepareActual(Client&,const OriginalMoldTrade&);
 bool RevalidateBeforePersist(Client&)const;
public:
 ~PreparedTerminalChestPlan();
 const std::vector<TerminalAcceptedMemberFacts>& AcceptedMembers()const noexcept;
 PreparedTerminalChestPlan(const PreparedTerminalChestPlan&)=delete;
 PreparedTerminalChestPlan& operator=(const PreparedTerminalChestPlan&)=delete;
};
struct TerminalDeathFacts {
 MissionRunFacts mission;
 uint64_t native_sequence=0,terminal_row=0;
 uint32_t npc_type=0,original_spawn=0,original_entity=0,corpse_entity=0;
 bool original_chest_open=false; // fixed by private original native open frame
};
class OriginalTerminalDeathProof final {
public:
 struct State; // opaque native witness state; no public constructor or issuer
private:
 friend class OriginalTerminalDeath;
 friend class NativeRunTerminalSource;
 friend class NativeRunTerminalCustody;
 TerminalDeathFacts facts_;
 bool taken_=false;
 std::shared_ptr<State> state_;
 OriginalTerminalDeathProof(TerminalDeathFacts,std::shared_ptr<State>);
public:
 ~OriginalTerminalDeathProof();
 const TerminalDeathFacts& Facts()const noexcept{return facts_;}
 OriginalTerminalDeathProof(const OriginalTerminalDeathProof&)=delete;
 OriginalTerminalDeathProof& operator=(const OriginalTerminalDeathProof&)=delete;
};
// Scope begins at the original actual NPC::Death before any callback and can
// seal only its own positive native return plus SAME registered corpse.
class OriginalTerminalDeath final {
 friend class ::TaskManager;
 friend class OriginalMissionMaterialDeath;
 friend class ::Corpse;
 friend class ::EntityList;
 friend class NativeRunTerminalSource;
 struct State;
 std::shared_ptr<State> state_;
 OriginalTerminalDeath* previous_=nullptr;
 static thread_local OriginalTerminalDeath* current_;
 static uint64_t ActualGuardianSequence(NPC&);
 static void ObserveActualKillVisit(Client&,NPC&);
 static void CaptureActualCorpse(Corpse&,NPC&);
 static void ObserveActualCorpseRegistered(Corpse&);
public:
 explicit OriginalTerminalDeath(NPC&)noexcept;
 ~OriginalTerminalDeath();
 void SealActualSuccess(NPC&,uint32_t original_entity);
};
enum class TerminalNativeBoundary:uint8_t {Registered=1,OpenedDeath=2};
struct TerminalNativeFacts {
 TerminalNativeBoundary boundary{};
 MissionRunFacts mission;
 uint64_t terminal_row=0,original_accept_sequence=0,native_death_sequence=0;
 uint32_t actor_entity=0,corpse_entity=0;
};
class NativeTerminalPublication final {
 friend class NativeRunTerminalSource;
 friend class NativeRunTerminalCustody;
 TerminalNativeFacts facts_;
 std::shared_ptr<RunTerminalAttempt> owner_;
 bool taken_=false;
 NativeTerminalPublication(TerminalNativeFacts,std::shared_ptr<RunTerminalAttempt>);
public:
 ~NativeTerminalPublication();
 const TerminalNativeFacts& Facts()const noexcept{return facts_;}
 NativeTerminalPublication(const NativeTerminalPublication&)=delete;
 NativeTerminalPublication& operator=(const NativeTerminalPublication&)=delete;
};
class PublishedTerminalReturnReceipt; // Core private one-use strict writer receipt
struct PublishedTerminalReturnView;
class PublishedTerminalReturnSink final {
public:
 static void Receive(Client&,PublishedTerminalReturnReceipt&);
};
class NativeRunTerminalSource final {
 friend class ::Client;
 friend class ::NPC;
 friend class ::Corpse;
 friend class ::EntityList;
 friend class ::Zone;
 friend class ::TaskManager;
 friend class PreparedTerminalChestPlan;
 friend class PublishedTerminalReturnSink;
 friend class OriginalTerminalDeath;
 friend class NativeRunLootSource;
 static bool AuthorizeAcknowledgedGuardianCorpse(Corpse&,const MissionRunFacts&,uint64_t original_death_sequence);
 static bool HandleOriginalAccept(Client&,NPC&); // before original finish/state/reset
 static bool RejectUnissuedFinishTrade(Client&,NPC&);
 static bool RevalidateOriginalReturn(Client&,const OriginalMoldTrade&);
 static std::shared_ptr<OriginalTerminalDeathProof::State> death_head_,death_tail_;
 static bool CaptureOriginalOpenDeath(NPC&,OriginalTerminalDeathProof::State&);
 static void BindOriginalOpenedCorpse(Corpse&,OriginalTerminalDeathProof::State&);
 static bool SealOriginalChestCorpse(OriginalTerminalDeathProof::State&);
 static bool AuthorizeActualChestOpen(Client&,NPC&);
 static bool HandleOriginalChestOpen(Client&,NPC&); // actual native open target
 static bool RejectOrdinaryChestDeath(NPC&);
 static void ObserveActualRegistration(NPC*);
 static void ObserveActualDestruction(NPC&);
 static std::optional<bool> ProcessActualChest(NPC&);
 static bool AuthorizeActualCorpse(Client&,Corpse&);
 static void DrainCompletions();
 static void DiscardCompletions()noexcept;
};
} // namespace FireRunActionSource
