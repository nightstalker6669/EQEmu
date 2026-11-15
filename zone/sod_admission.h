#pragma once
#include "common/sod_admission.h"
#include "zone/client.h"
#include "zone/merc.h"
#include "zone/groups.h"
#include "zone/zone.h"
#include "common/shared_tasks.h"
#include <atomic>
namespace SodAdmission {
inline std::optional<Proof> Observe(Client& c,uint32_t task) {
 if(!SodRewardQuote::Shared(task)||!zone)return std::nullopt;
 auto request=SharedTask::GetRequestCharacters(database,c.CharacterID());
 if(request.members.empty()||request.members.size()>6)return std::nullopt;
 Proof p;p.zone=zone->GetZoneID();p.instance=zone->GetInstanceID();p.requester=c.CharacterID();p.task=task;
 p.created_ms=Milliseconds();static std::atomic<uint64_t> serial{0};p.nonce=(p.created_ms<<16)^(++serial);
 for(const auto& member:request.members)p.players[p.player_count++]=member.character_id;
 auto* group=c.GetGroup();
 if(group)for(unsigned i=0;i<6;++i) {
  auto* mob=group->members[i];if(!mob||!mob->IsMerc())continue;
  auto* merc=mob->CastToMerc();auto* owner=merc->GetMercenaryOwner();
  if(!owner||merc->IsDead()||merc->IsSuspended()||merc->GetGroup()!=group||owner->GetGroup()!=group||
   owner->GetZoneID()!=p.zone||owner->GetInstanceID()!=p.instance||owner->GetHP()<=0)continue;
  p.mercs[p.merc_count]=merc->GetMercenaryID();p.owners[p.merc_count++]=owner->CharacterID();
 }
 return Shape(p)&&(p.player_count+p.merc_count>=3||c.GetGM())?std::optional<Proof>(p):std::nullopt;
}
}
