#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <type_traits>
class Client;
class Database;
namespace FirePersonalOccurrenceSource {class NativeItemCustody;}
namespace FireRunActionSource {
// Original native use records and preallocated scene/actor plans belong to
// Spells. Constructors/capture/revalidation stay private to genuine native seams.
class OriginalDoorUse;
class OriginalHelperUse;
class PreparedRunDoorPlan;
class PreparedRunHelperPlan;
class NativeRunDoorSource;
class PublishedRunActionSink;
class NativeRunHelperSource;
class NativeRunPublication;
struct RunLatchSnapshot;
class PreparedRunActionOrigins;
enum class RunActionKind : uint8_t {
 InnerVault=1,WorshipPair=2,GuardianVaultKey=3,HelperActivation=4,WorldFontKey=5
};
enum class RunActionBoundary : uint8_t {
 CommittedFirstRunAction,ExistingPublishedRunLatch,RetainedWorldPhysicalKey
};
struct PublishedRunActionView {
 RunActionKind kind;
 RunActionBoundary boundary;
 uint64_t run_action_id; // real inserted/read row ID; zero ONLY for world key use
 uint64_t run_id; // real shared run; zero ONLY for world key use
 uint64_t native_sequence;
 const OriginalDoorUse* door_original;
 const OriginalHelperUse* helper_original;
 PreparedRunDoorPlan* door_plan;
 PreparedRunHelperPlan* helper_plan;
 // All pointers borrowed from the issuing receipt, never client click state.
};
static_assert(std::is_trivially_copyable<PublishedRunActionView>::value,
 "Run-action views cannot allocate or mint authority");
class NativeRunActionCustody;
class PublishedRunActionReceipt final {
 friend class NativeRunActionCustody;
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 struct State;
 std::unique_ptr<State> state_; // fully allocated and pinned BEFORE persistence
 explicit PublishedRunActionReceipt(std::unique_ptr<State>) noexcept;
public:
 ~PublishedRunActionReceipt();
 PublishedRunActionReceipt(const PublishedRunActionReceipt&)=delete;
 PublishedRunActionReceipt& operator=(const PublishedRunActionReceipt&)=delete;
 PublishedRunActionReceipt(PublishedRunActionReceipt&&) noexcept;
 PublishedRunActionReceipt& operator=(PublishedRunActionReceipt&&)=delete;
 std::optional<PublishedRunActionView> Take(const Client& actual_owner) noexcept;
};
enum class DoorActionResult : uint8_t {SelectedRejected,CommittedToNativePublication};
class NativeRunActionCustody final {
 friend class NativeRunDoorSource;
 friend class NativeRunHelperSource;
 friend class PublishedRunActionSink;
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 // Called ONLY by the original door click's private issuer, before mutable
 // door/quest callbacks. This is a Door event, never a fabricated spell finish.
 // Consumes an owned original intent and preallocated native scene plan.
 // A selected reject MUST return from the native door event; never fall back
 // to generic door/keyring logic. Caller returns immediately after this call
 // and does not touch Client/door after the mutable final publication sink.
 static DoorActionResult ExecuteOriginalDoor(Client&,std::unique_ptr<OriginalDoorUse>,
  std::unique_ptr<PreparedRunDoorPlan>);
 static bool RevalidateDoorPlan(Client&,const PreparedRunDoorPlan&);
 static std::unique_ptr<RunLatchSnapshot> PreviewActual(const OriginalDoorUse&);
 static std::unique_ptr<RunLatchSnapshot> PreviewActual(const OriginalHelperUse&);
 static uint64_t StageActual(Database&,uint64_t connection,const RunLatchSnapshot&,
  const OriginalDoorUse*,const OriginalHelperUse*,PreparedRunActionOrigins*,
  uint64_t native_sequence,bool& write_entered);
 static bool AcknowledgeNativePublication(NativeRunPublication&);
 // Helper47756 stays on the existing genuine successful-cast/debit ticket;
 // helper preparation and strict writer integration are feature-private.
};
// Final mutable native publication sink. It takes/seals the issuing receipt
// first and transfers the SAME planned scene/helper. No Client/state access
// follows this call in a custody issuer. Native scene acknowledgments belong to
// private original door/walk/arrival/barrier/dust receipts, never this view alone.
void OnOriginalRunActionPublished(Client&,PublishedRunActionReceipt&);
} // namespace FireRunActionSource
