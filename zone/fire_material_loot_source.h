#pragma once
#include "fire_run_action_scene.h"
#include <cstdint>
#include <memory>
class Corpse;
class EntityList;
namespace FireRunActionSource {
class NativeRunItemOrigins; // Core actual finite seal/origin persistence
class PreparedRunLootSeal; // Core preallocated pending native seal object
class MissionMaterialCorpseOrigin;
class NativeRunLootSource;
struct RunLootFacts {
 MissionRunFacts mission; // actual looter own accepted mapping, not NPC/run0 binding
 uint64_t native_sequence=0,source_death_sequence=0;
 uint32_t source_npc_type=0,source_spawn=0,source_corpse_entity=0;
 uint16_t source_loot_slot=0;
 uint32_t item_id=0;
 int32_t native_item_serial=0;
 int16_t charges=0,saved_slot=-1;
};
class OriginalRunLootUse final {
 friend class NativeRunLootSource;
 friend class NativeRunItemOrigins;
 friend class NativeRunLootPublication;
 struct State;
 RunLootFacts facts_;
 std::shared_ptr<State> state_;
 OriginalRunLootUse(RunLootFacts,std::shared_ptr<State>);
public:
 ~OriginalRunLootUse();
 const RunLootFacts& Facts()const noexcept{return facts_;}
 OriginalRunLootUse(const OriginalRunLootUse&)=delete;
 OriginalRunLootUse& operator=(const OriginalRunLootUse&)=delete;
};
class NativeRunLootPublication final {
 friend class NativeRunLootSource;
 friend class NativeRunItemOrigins;
 RunLootFacts facts_;
 bool taken_=false;
 std::unique_ptr<PreparedRunLootSeal> seal_;
 struct State;
 std::unique_ptr<State> state_;
 explicit NativeRunLootPublication(std::unique_ptr<PreparedRunLootSeal>,std::unique_ptr<State>)noexcept;
public:
 ~NativeRunLootPublication();
 const RunLootFacts& Facts()const noexcept{return facts_;}
 NativeRunLootPublication(const NativeRunLootPublication&)=delete;
 NativeRunLootPublication& operator=(const NativeRunLootPublication&)=delete;
};
// Real successful NPC::Death -> SAME actually constructed/registered Corpse.
// A later type/zone/name lookup cannot mint this lineage.
class OriginalMissionMaterialDeath final {
 friend class ::NPC;
 friend class ::Corpse;
 friend class ::EntityList;
 struct State;
 std::shared_ptr<State> state_;
 OriginalMissionMaterialDeath* previous_=nullptr;
 static thread_local OriginalMissionMaterialDeath* current_;
 explicit OriginalMissionMaterialDeath(NPC&)noexcept;
 void SealActualSuccess(NPC&,uint32_t actual_killed_entity);
 static void CaptureActualCorpse(Corpse&,NPC&);
 static void ObserveActualCorpseRegistered(Corpse&);
public:
 ~OriginalMissionMaterialDeath();
 OriginalMissionMaterialDeath(const OriginalMissionMaterialDeath&)=delete;
 OriginalMissionMaterialDeath& operator=(const OriginalMissionMaterialDeath&)=delete;
};
// Scoped only to actual native Corpse::LootCorpseItem. Preparation happens after
// ordinary loot permission/selection, before original inventory grant; observer
// reads SAME original SaveInventory/SaveCursor result and actual removal.
class NativeRunLootSource final {
 friend class ::Corpse;
 friend class ::Client;
 struct State;
 std::shared_ptr<State> state_;
 bool scene_candidate_=false;
 uint32_t source_npc_type_=0;
 NativeRunLootSource* previous_=nullptr;
 static thread_local NativeRunLootSource* current_;
 explicit NativeRunLootSource(Client&,Corpse&)noexcept;
 bool PrepareActualItem(EQ::ItemInstance&,uint16_t actual_loot_slot);
 static void ObserveActualInventorySave(Client&,const EQ::ItemInstance&,int16_t actual_slot,
  bool same_existing_save_result);
 void ObserveActualRemoval(Corpse&,uint16_t actual_removed_slot);
public:
 ~NativeRunLootSource();
 NativeRunLootSource(const NativeRunLootSource&)=delete;
 NativeRunLootSource& operator=(const NativeRunLootSource&)=delete;
};
} // namespace FireRunActionSource
