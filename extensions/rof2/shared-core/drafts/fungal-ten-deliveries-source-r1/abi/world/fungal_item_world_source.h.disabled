#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "../common/fungal_item_operation_wire.h"
#include "../common/fungal_world_hold.h" // supplied by the exact Core owner component
#include "../common/repositories/shared_task_activity_state_repository.h"
class Database;
class ServerPacket;
class SharedTask;
class SharedTaskManager;
class SharedTaskWorldMessaging;
namespace FungalInventory {class WorldCustody;class WorldTransaction;}
namespace FungalItemWorldSource {
class NativeStage;
class PacketScope final {
 friend class ::SharedTaskWorldMessaging;
 friend class NativeStage;
 PacketScope* previous_=nullptr;
 static thread_local PacketScope* current_;
 FungalItemWire::Trailer original_{};
 uint32_t character_=0,activity_=0,count_=0;
 bool selected_=false,valid_=false;
 explicit PacketScope(const ServerPacket&);
public:
 ~PacketScope();
 PacketScope(const PacketScope&)=delete;
 PacketScope& operator=(const PacketScope&)=delete;
};
class OriginalWorldUpdate final {
 friend class NativeStage;
 friend class FungalInventory::WorldCustody;
 FungalItemWire::Trailer original_{};
 uint32_t activity_=0,count_=0;
 SharedTask* actual_=nullptr;
 OriginalWorldUpdate()=default;
public:
 OriginalWorldUpdate(const OriginalWorldUpdate&)=delete;
 OriginalWorldUpdate& operator=(const OriginalWorldUpdate&)=delete;
};
class KnownStageAck final {
 friend class FungalInventory::WorldCustody;
 friend class NativeStage;
 FungalItemWire::Trailer original_{};
 uint32_t activity_=0,count_=0;
 KnownStageAck(FungalItemWire::Trailer f,uint32_t a,uint32_t c):original_(f),activity_(a),count_(c){}
public:
 ~KnownStageAck()=default;
 KnownStageAck(const KnownStageAck&)=delete;
 KnownStageAck& operator=(const KnownStageAck&)=delete;
};
class NativeStage final {
 friend class ::SharedTaskManager;
 static thread_local NativeStage* current_;
 NativeStage* previous_=nullptr;
 bool selected_=false;
 std::unique_ptr<OriginalWorldUpdate> original_;
 std::unique_ptr<FungalInventory::WorldTransaction> transaction_;
 std::unique_ptr<KnownStageAck> ack_;
 NativeStage(Database&,SharedTask*,uint32_t character,uint32_t task,uint32_t activity,uint32_t count);
public:
 ~NativeStage();
 NativeStage(const NativeStage&)=delete;
 NativeStage& operator=(const NativeStage&)=delete;
 bool Selected()const noexcept{return selected_;}
 bool Ready()const noexcept{return bool(transaction_);}
 bool Committed()const noexcept{return bool(ack_);}
 static bool SelectedSave(uint64_t run);
 static bool CommitActualSave(Database&,uint64_t run,
  const std::vector<SharedTaskActivityStateRepository::SharedTaskActivityState>&);
 static bool StampActualAck(uint64_t run,uint32_t activity,FungalItemWire::Trailer&);
};
}
namespace FungalInventory {
// Core owns the ONLY strict transaction implementation; the source frame
// exists only inside the genuine native World shared update/save call chain.
class WorldTransaction { public: virtual ~WorldTransaction()=default; };
class WorldCustody final {
public:
 static std::unique_ptr<WorldTransaction> Prepare(Database&,
  const FungalItemWorldSource::OriginalWorldUpdate&);
 static std::unique_ptr<FungalItemWorldSource::KnownStageAck> CommitActualSave(
  Database&,WorldTransaction&,const FungalItemWorldSource::OriginalWorldUpdate&,
  const std::vector<SharedTaskActivityStateRepository::SharedTaskActivityState>&);
};
}
