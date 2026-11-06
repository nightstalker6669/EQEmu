#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
class Client;
class Database;
namespace EQ {class ItemInstance;}
namespace FirePersonalOccurrenceSource {class NativeItemCustody;}
namespace FireSharedTransformSource {struct OriginalSharedTransformUse;}
namespace FireRunActionSource {
struct MissionRunFacts;
class OriginalRunLootUse; // Spells: private genuine current-run corpse loot issuer
class NativeRunLootPublication; // Spells: actual positive native Save + corpse removal
class NativeRunLootSource;
class NativeRunDoorSource;
class NativeRunHelperSource;
class NativeRunActionCustody;
class OriginalDoorUse;
class OriginalHelperUse;
class PreparedRunActionOrigins final {
 friend class NativeRunItemOrigins;
 struct State;
 std::unique_ptr<State> state_;
 explicit PreparedRunActionOrigins(std::unique_ptr<State>) noexcept;
public:
 ~PreparedRunActionOrigins();
 PreparedRunActionOrigins(const PreparedRunActionOrigins&)=delete;
 PreparedRunActionOrigins& operator=(const PreparedRunActionOrigins&)=delete;
};
class PreparedRunTransformOrigins final {
 friend class NativeRunItemOrigins;
 struct State;
 std::unique_ptr<State> state_;
 explicit PreparedRunTransformOrigins(std::unique_ptr<State>) noexcept;
public:
 ~PreparedRunTransformOrigins();
 PreparedRunTransformOrigins(const PreparedRunTransformOrigins&)=delete;
 PreparedRunTransformOrigins& operator=(const PreparedRunTransformOrigins&)=delete;
};
inline constexpr const char* RunOriginSealKey="rof2.fire170.origin.v1";
class PreparedRunLootSeal final {
 friend class NativeRunItemOrigins;
 struct State;
 std::unique_ptr<State> state_;
 explicit PreparedRunLootSeal(std::unique_ptr<State>) noexcept;
public:
 ~PreparedRunLootSeal();
 PreparedRunLootSeal(const PreparedRunLootSeal&)=delete;
 PreparedRunLootSeal& operator=(const PreparedRunLootSeal&)=delete;
 // Read-only seal identity for the SAME owned native loot lifecycle. A token
 // alone is never publication, physical ownership or permission to consume.
 const std::string& Token()const noexcept;
};
class NativeRunItemOrigins final {
 friend class NativeRunLootSource;
 friend class NativeRunDoorSource;
 friend class NativeRunHelperSource;
 friend class NativeRunActionCustody;
 friend class FirePersonalOccurrenceSource::NativeItemCustody;
 // Actual newly created original loot instance ONLY, before its original save.
 // Stamps native custom_data; no DB row or publication is manufactured here.
 static std::unique_ptr<PreparedRunLootSeal> SealOriginalLoot(
  std::unique_ptr<OriginalRunLootUse>,EQ::ItemInstance& actual_new_item);
 // Seals the Source-owned one-use positive save+removal proof FIRST, then
 // persists the finite published origin. No tokenless/history/backfill path.
 static bool PublishOriginalLoot(NativeRunLootPublication&);
 // Actual current holder/root + sealed physical item + published current-run
 // row. Creator is audit metadata, never the current-holder ownership check.
 static bool HasActualPublishedOrigin(Client&,const EQ::ItemInstance&,
  int16_t actual_slot,const MissionRunFacts&);
 static std::unique_ptr<PreparedRunActionOrigins> PrepareActionActual(Client&,
  const OriginalDoorUse*,const OriginalHelperUse*);
 static bool RevalidateActionActual(Client&,const PreparedRunActionOrigins&);
 static std::string LockActionActual(Database&,uint64_t connection,
  PreparedRunActionOrigins&,const OriginalDoorUse*,const OriginalHelperUse*);
 static void ConsumeActionActual(Database&,uint64_t connection,
  PreparedRunActionOrigins&,uint64_t actual_action_row);
 static std::unique_ptr<PreparedRunTransformOrigins> PrepareTransformActual(Client&,
  const FireSharedTransformSource::OriginalSharedTransformUse&,
  const std::vector<int16_t>& actual_input_slots,EQ::ItemInstance& actual_new_output,
  uint64_t actual_sequence,int16_t actual_output_slot);
 static bool RevalidateTransformActual(Client&,const PreparedRunTransformOrigins&);
 // Caller is the same native strict writer, before its selected inventory saves.
 // Consumes inputs and inserts pending output on THAT connection/transaction.
 static void StageTransformActual(Database&,uint64_t connection,
  PreparedRunTransformOrigins&,bool& write_entered);
 // SAME known successful COMMIT and actual allocation-free memory publication,
 // before mutable callbacks. A failed/unknown acknowledgment leaves pending0.
 static bool PublishTransformActual(Client&,PreparedRunTransformOrigins&,
  const EQ::ItemInstance& actual_published_output,int16_t actual_output_slot);
};
} // namespace FireRunActionSource
