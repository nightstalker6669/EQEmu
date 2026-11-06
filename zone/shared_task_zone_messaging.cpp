#include "../common/fungal_item_operation_wire.h"
#include "fungal_item_operation_source.h"
/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "shared_task_zone_messaging.h"

#include "common/servertalk.h"
#include "common/shared_tasks.h"
#include "zone/client.h"
#include "fire_shared_successor_native.h"
#include "common/fire_shared_activity_origin.h"
#include "zone/task_client_state.h"
#include "zone/task_manager.h"
#include "zone/npc.h"
#include "zone/dynamic_zone.h"
#include "zone/zonedb.h"
#include "task_shared_reward_identity.h"

#include <memory>
#include <vector>

namespace FirePersonalOccurrenceSource {
namespace {
thread_local const SharedSuccessorPending* active_fire_shared_request=nullptr;
struct SharedRequestScope {
 const SharedSuccessorPending* previous;
 explicit SharedRequestScope(const SharedSuccessorPending* p):previous(active_fire_shared_request) {active_fire_shared_request=p;}
 ~SharedRequestScope(){active_fire_shared_request=previous;}
};
bool SavedLocalShared(Database& db,const SharedSuccessorPending& s,const ClientTaskInformation& current) {
 if(current.task_id!=920170 || current.accepted_time!=static_cast<int>(s.accepted.accept_time) ||
  current.was_rewarded || !s.local_occurrence) return false;
 auto r=db.QueryDatabase(fmt::format(
  "SELECT 1 FROM character_tasks c JOIN character_task_reward_instances r ON r.character_id=c.charid "
  "AND r.task_id=c.taskid AND r.accepted_time=c.acceptedtime "
  "JOIN character_shared_task_reward_instances h ON h.character_id=c.charid AND h.task_id=c.taskid "
  "AND h.occurrence_id=r.occurrence_id WHERE c.charid={} AND c.taskid=920170 AND c.type={} "
  "AND c.slot={} AND c.acceptedtime={} AND c.was_rewarded=0 AND r.occurrence_id={} AND h.shared_task_id={}",
  s.source.character_id,static_cast<int>(TaskType::Shared),current.slot,current.accepted_time,
  s.local_occurrence,s.world.run));
 return r.Success() && r.RowCount()==1;
}
}
bool SharedSuccessorNative::Match(const SharedSuccessorPending& s,const ServerSharedTaskRequest_Struct& p,
 const FireSharedCorrelation::Trailer& t) {
 return p.requested_character_id==s.source.character_id && p.requested_task_id==920170 &&
  p.requested_npc_entity_id==s.giver && t.source_occurrence==s.source.occurrence_id &&
  t.pending_reward==s.reward;
}
CorrelatedSharedResult SharedSuccessorNative::Dispatch(Handoff& handoff,const Binding& old,Client& c,NPC& npc,
 uint32_t actual_zone,uint32_t actual_instance,uint32_t spawn) {
 if(active_fire_shared_request || old.task_id!=920169 || !old.occurrence_id || !old.character_id ||
  old.slot<0 || old.accepted_time<=0 || old.character_id!=c.CharacterID() || !c.GetTaskState() ||
  c.m_requesting_shared_task || actual_zone!=490 || actual_instance || !spawn ||
  npc.GetNPCTypeID()!=490061 || !npc.GetID() || npc.GetSpawnPointID()!=spawn ||
  (c.m_fire_shared_successor && (!c.m_fire_shared_successor->ready ||
   c.m_fire_shared_successor->source.occurrence_id==old.occurrence_id))) return CorrelatedSharedResult::Hold;
 // Current normal membership is not a new Ready/origin receipt.
 if(c.GetTaskState()->GetActiveSharedTask().task_id==920170)return CorrelatedSharedResult::Hold;
 const auto* next=TaskManager::Instance()->GetTaskData(920170);
 if(!next || next->type!=TaskType::Shared) return CorrelatedSharedResult::Hold;
 auto pending=std::make_unique<SharedSuccessorPending>();
 auto intent=TakeOriginalHandoffOnce(handoff);
 if(!intent || intent->next_task!=920170 || intent->next_type!=TaskType::Shared || !intent->pending_reward_id ||
  intent->finished.character_id!=old.character_id || intent->finished.task_id!=old.task_id ||
  intent->finished.occurrence_id!=old.occurrence_id || intent->finished.slot!=old.slot ||
  intent->finished.accepted_time!=old.accepted_time) return CorrelatedSharedResult::Hold;
 pending->source=old;pending->reward=intent->pending_reward_id;
 pending->giver=npc.GetID();pending->spawn=spawn;pending->original_actor=&npc;
 c.m_fire_shared_successor=std::move(pending);
 SharedRequestScope scope(c.m_fire_shared_successor.get());
 c.AssignTask(920170,npc.GetID(),true);
 return CorrelatedSharedResult::NativeInvocationUnconfirmed; // stack-only after call
}
bool SharedSuccessorNative::Stamp(Client& c,ServerSharedTaskRequest_Struct& p,FireSharedCorrelation::Trailer& t) {
 if(p.requested_task_id!=920170 || !c.m_fire_shared_successor || c.m_fire_shared_successor->ready) return true;
 auto& s=*c.m_fire_shared_successor;
 if(active_fire_shared_request!=&s || s.held || s.stamped || p.requested_character_id!=s.source.character_id ||
  p.requested_npc_entity_id!=s.giver) return false;
 s.stamped=true;t.source_occurrence=s.source.occurrence_id;t.pending_reward=s.reward;
 return true;
}
bool SharedSuccessorNative::BeginAccept(Client& c,const ServerSharedTaskRequest_Struct& p,
 const FireSharedCorrelation::Trailer& t) {
 auto* s=c.m_fire_shared_successor.get();
 if(!s || !s->stamped || s->held || s->accept_claimed || !Match(*s,p,t) ||
  t.phase!=FireSharedCorrelation::Admitted || t.dz || t.instance || !zone ||
  zone->GetZoneID()!=490 || zone->GetInstanceID()) return false;
 s->accept_claimed=true; // seal before DB reads/mutable task acceptance
 if(!c.GetTaskState()) {s->held=true;return false;}
 auto* n=entity_list.GetNPCByID(s->giver);
 if(!n || n!=s->original_actor || n->GetNPCTypeID()!=490061 || n->GetSpawnPointID()!=s->spawn ||
  !FireSharedCorrelation::HeaderAndRoster(database,p,t)) {s->held=true;return false;}
 s->accepted=p;s->world=t;return true;
}
void SharedSuccessorNative::ObserveReservation(Client& c,int task,uint32_t accepted,uint64_t occurrence) {
 auto* s=c.m_fire_shared_successor.get();
 if(!s || !s->accept_claimed || s->held || s->save_seen || task!=920170 ||
  !c.m_requesting_shared_task || accepted!=s->accepted.accept_time || !occurrence) return;
 s->local_occurrence=occurrence; // SAME existing successful native shared Ensure result
}
void SharedSuccessorNative::ObserveSave(Client& c,TaskType type,const ClientTaskInformation& current,
 bool actual_save,int activity_count) {
 auto* s=c.m_fire_shared_successor.get();
 if(!s || !s->accept_claimed || s->held || s->save_seen || current.task_id!=920170 ||
  !c.m_requesting_shared_task) return;
 s->save_seen=true;
 if(type!=TaskType::Shared || !actual_save || activity_count!=19 || !SavedLocalShared(database,*s,current) ||
  !FireSharedCorrelation::HeaderAndRoster(database,s->accepted,s->world)) {s->held=true;return;}
 auto r=database.QueryDatabase(fmt::format(
  "SELECT COUNT(*) FROM character_activities WHERE charid={} AND taskid=920170 "
  "AND activityid BETWEEN 0 AND 18 AND donecount=0 AND completed=0",c.CharacterID()));
 if(!r.Success() || r.RowCount()!=1) {s->held=true;return;}
 auto row=r.begin();
 if(!row[0] || std::string(row[0])!="19") {s->held=true;return;}
 s->local_saved=true; // before mutable TASK_ACCEPTED; not a post-callback view
}
void SharedSuccessorNative::Finish(Client& c) {
 auto* s=c.m_fire_shared_successor.get();
 if(!s || s->held || s->ready || !s->local_saved || !s->reloaded || !s->ready_received) return;
 auto* state=c.GetTaskState();
 if(!state || c.GetSharedTaskId()<=0 || static_cast<uint64_t>(c.GetSharedTaskId())!=s->world.run ||
  !SavedLocalShared(database,*s,state->GetActiveSharedTask()) ||
  !FireSharedCorrelation::PersistedMissionReady(database,s->accepted,s->world)) {s->held=true;return;}
 bool matched=false;
 for(auto* dz:c.GetDynamicZones(490,1)) {
  if(dz && dz->GetID()==s->world.dz && dz->GetInstanceID()==s->world.instance &&
   dz->GetType()==DynamicZoneType::Mission && dz->IsValid() && !dz->IsExpired() &&
   dz->HasMember(c.CharacterID()) && dz->HasDatabaseMember(c.CharacterID())) matched=true;
 }
 if(!matched) {s->held=true;return;}
 s->ready=true; // private exact original source->actual run/roster/DZ result; no travel/reward
}
void SharedSuccessorNative::AfterReload(Client& c) {
 auto* s=c.m_fire_shared_successor.get();
 if(!s || !s->accept_claimed || s->held) return;
 s->reloaded=true;Finish(c);
}
void SharedSuccessorNative::ReadyOrHeld(Client& c,const ServerSharedTaskRequest_Struct& p,
 const FireSharedCorrelation::Trailer& t) {
 auto* s=c.m_fire_shared_successor.get();
 if(!s || !s->stamped || s->held || s->ready || !Match(*s,p,t)) return;
 if(t.phase==FireSharedCorrelation::Held) {s->held=true;return;}
 if(t.phase!=FireSharedCorrelation::Ready || !s->accept_claimed || s->ready_received ||
  p.accept_time!=s->accepted.accept_time || t.run!=s->world.run ||
  t.count!=s->world.count || !std::equal(std::begin(t.members),std::end(t.members),std::begin(s->world.members)) ||
  !FireSharedCorrelation::PersistedMissionReady(database,p,t)) {s->held=true;return;}
 s->world=t;s->ready_received=true;Finish(c);
}
} // namespace FirePersonalOccurrenceSource

void SharedTaskZoneMessaging::HandleWorldMessage(ServerPacket *pack)
{
	switch (pack->opcode) {
		case ServerOP_SharedTaskAcceptNewTask: {
			FireSharedCorrelation::Trailer fire_reply;
			const auto kind=FireSharedCorrelation::Decode(pack->pBuffer,pack->size,fire_reply);
			if(kind==FireSharedCorrelation::Kind::Invalid) break;
			auto p = reinterpret_cast<ServerSharedTaskRequest_Struct *>(pack->pBuffer);
			auto c = entity_list.GetClientByCharID(p->requested_character_id);
			if (c) {
				LogTasks("[ServerOP_SharedTaskAcceptNewTask] We're back in zone and I found [{}]", c->GetCleanName());

				if(kind==FireSharedCorrelation::Kind::Fire) {
					if(fire_reply.phase!=FireSharedCorrelation::Admitted) {
						FirePersonalOccurrenceSource::SharedSuccessorNative::ReadyOrHeld(*c,*p,fire_reply);break;
					}
					if(!FirePersonalOccurrenceSource::SharedSuccessorNative::BeginAccept(*c,*p,fire_reply)) break;
				}
				c->m_requesting_shared_task = true;
				c->GetTaskState()
					->AcceptNewTask(
						c,
						(int) p->requested_task_id,
						(int) p->requested_npc_entity_id,
						p->accept_time
					);
				c->LoadClientTaskState();
				c->m_requesting_shared_task = false;
				if(kind==FireSharedCorrelation::Kind::Fire)
					FirePersonalOccurrenceSource::SharedSuccessorNative::AfterReload(*c);
			}

			break;
		}
		case ServerOP_SharedTaskUpdate: {
			if(pack->size<sizeof(ServerSharedTaskActivityUpdate_Struct)) break;
			auto p = reinterpret_cast<ServerSharedTaskActivityUpdate_Struct *>(pack->pBuffer);
			uint64_t original_run=0;
			if(!FungalItemWire::Correlated(p->task_id,p->activity_id)&&
			 !FireSharedActivityOrigin::Read(pack->pBuffer,pack->size,p->task_id,original_run)) break;
			auto c = entity_list.GetClientByCharID(p->source_character_id);
			if (c) {
				LogTasks("[ServerOP_SharedTaskUpdate] We're back in zone and I found [{}]", c->GetCleanName());

				if(p->task_id==FireSharedActivityOrigin::Task) {
					auto* state=c->GetTaskState();
					if(!state || state->GetActiveSharedTask().task_id!=static_cast<int>(p->task_id) ||
					 c->GetSharedTaskId()<=0 || static_cast<uint64_t>(c->GetSharedTaskId())!=original_run) break;
					const auto actual=TaskSharedRewardIdentity::ReadRun(database,c->CharacterID(),p->task_id);
					if(!actual || actual->id!=original_run || actual->accepted_time!=
					 static_cast<uint32_t>(state->GetActiveSharedTask().accepted_time)) break;
				}
				if(!FungalItemSource::NativeEvents::AcceptWorldEcho(*c,pack->pBuffer,pack->size,p->task_id,p->activity_id,p->done_count))break;
				c->m_shared_task_update = true;
				c->GetTaskState()->SharedTaskIncrementDoneCount(
					c,
					(int) p->task_id,
					(int) p->activity_id,
					(int) p->done_count,
					p->ignore_quest_update
				);
				c->m_shared_task_update = false;
			}

			break;
		}
		case ServerOP_SharedTaskQuit: {
			auto p = reinterpret_cast<ServerSharedTaskQuit_Struct *>(pack->pBuffer);
			auto c = entity_list.GetClientByCharID(p->requested_character_id);
			if (c) {
				LogTasks("[ServerOP_SharedTaskQuit] We're back in zone and I found [{}]", c->GetCleanName());

				c->m_requested_shared_task_removal = true;
				c->GetTaskState()->CancelTask(
					c,
					TASKSLOTSHAREDTASK,
					static_cast<TaskType>((int) TASK_TYPE_SHARED),
					p->remove_from_db
				);
				c->m_requested_shared_task_removal = false;

				c->SetSharedTaskId(0);
			}

			break;
		}
		case ServerOP_SharedTaskMemberlist: {
			auto p = reinterpret_cast<ServerSharedTaskMemberListPacket_Struct *>(pack->pBuffer);

			LogTasks(
				"[ServerOP_SharedTaskMemberlist] We're back in zone and I'm searching for [{}]",
				p->destination_character_id
			);

			// find character and route packet
			auto c = entity_list.GetClientByCharID(p->destination_character_id);
			if (c) {
				LogTasks("[ServerOP_SharedTaskMemberlist] We're back in zone and I found [{}]", c->GetCleanName());

				std::vector<SharedTaskMember> members;

				// deserialize members from world
				EQ::Util::MemoryStreamReader ss(p->cereal_serialized_members, p->cereal_size);
				cereal::BinaryInputArchive   archive(ss);
				archive(members);

				SerializeBuffer buf(sizeof(SharedTaskMemberList_Struct) + 15 * members.size());
				buf.WriteInt32(0); // unknown ids
				buf.WriteInt32(0);
				buf.WriteInt32((int32) members.size());

				for (auto &m : members) {
					buf.WriteString(m.character_name);
					buf.WriteInt32(0); // monster mission
					buf.WriteInt8(m.is_leader ? 1 : 0);
				}

				auto outapp = std::make_unique<EQApplicationPacket>(OP_SharedTaskMemberList, std::move(buf));
				c->QueuePacket(outapp.get());
			}

			break;
		}
		case ServerOP_SharedTaskMemberChange: {
			auto p = reinterpret_cast<ServerSharedTaskMemberChangePacket_Struct *>(pack->pBuffer);

			LogTasksDetail("[ServerOP_SharedTaskMemberChange] Searching for [{}]", p->destination_character_id);

			auto c = entity_list.GetClientByCharID(p->destination_character_id);
			if (c) {
				LogTasksDetail("[ServerOP_SharedTaskMemberChange] Found [{}]", c->GetCleanName());

				SerializeBuffer buf;
				buf.WriteInt32(0);                 // unique character id of receiver, leave 0 for emu
				buf.WriteInt32(0);                 // unknown, seen 50, 4, 0
				buf.WriteInt8(p->removed ? 0 : 1); // 0: removed 1: added
				buf.WriteString(p->player_name);

				// live sends more after the name but it might just be garbage from
				// a re-used buffer (possibly a former name[64] buffer?)

				auto outapp = std::make_unique<EQApplicationPacket>(OP_SharedTaskMemberChange, std::move(buf));
				c->QueuePacket(outapp.get());
			}

			break;
		}
		case ServerOP_SharedTaskInvitePlayer: {
			auto p = reinterpret_cast<ServerSharedTaskInvitePlayer_Struct *>(pack->pBuffer);
			auto c = entity_list.GetClientByCharID(p->requested_character_id);
			if (c) {
				LogTasks("[ServerOP_SharedTaskInvitePlayer] We're back in zone and I found [{}]", c->GetCleanName());

				// init packet
				auto outapp = new EQApplicationPacket(OP_SharedTaskInvite, sizeof(SharedTaskInvite_Struct));
				auto *i     = (SharedTaskInvite_Struct *) outapp->pBuffer;

				// fill
				i->unknown00 = 0;
				i->invite_id = (int) p->invite_shared_task_id;
				strn0cpy(i->inviter_name, p->inviter_name, 64);
				strn0cpy(i->task_name, p->task_name, 64);

				// sends
				c->QueuePacket(outapp);
				safe_delete(outapp);
			}

			break;
		}
		case ServerOP_SharedTaskPurgeAllCommand: {
			LogTasksDetail("[ServerOP_SharedTaskPurgeAllCommand] Syncing clients");

			for (auto &client: entity_list.GetClientList()) {
				Client *c = client.second;
				TaskManager::Instance()->SyncClientSharedTaskState(c, c->GetTaskState());
				c->RemoveClientTaskState();
				c->LoadClientTaskState();
			}

			break;
		}
		case ServerOP_SharedTaskFailed: {
			auto buf = reinterpret_cast<ServerSharedTaskCharacterTask_Struct*>(pack->pBuffer);
			Client* client = entity_list.GetClientByCharID(buf->character_id);
			if (client)
			{
				client->SendTaskFailed(buf->task_id, TASKSLOTSHAREDTASK, TaskType::Shared);
			}
			break;
		}
		default:
			break;
	}

}
