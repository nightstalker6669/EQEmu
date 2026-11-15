#pragma once
#include "common/shared_task_admission.h"
namespace SodContent {
inline std::optional<uint32_t> Role(Database& content,uint32_t task,const std::string& role) {
 auto r=content.QueryDatabase(fmt::format("SELECT npc_type_id FROM sod_scene_actor_bindings WHERE task_id={} AND role='{}'",task,Strings::Escape(role)));
 if(!r.Success()||r.RowCount()!=1)return std::nullopt;
 auto n=SharedTaskAdmission::Number(r.begin()[0]);return n&&*n&&*n<=UINT32_MAX?std::optional<uint32_t>(*n):std::nullopt;
}
}
