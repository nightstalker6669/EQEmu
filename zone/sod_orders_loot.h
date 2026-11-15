#pragma once
#include "zone/client.h"
#include "zone/corpse.h"
#include "zone/sod_progression.h"
#include "common/sod_content_bindings.h"
namespace SodOrdersLoot {
struct Ticket { Client* client;const EQ::ItemInstance* item;uint32_t serial;Corpse* corpse;bool consumed=false; };
inline thread_local Ticket* active=nullptr;
struct Scope {
 Ticket ticket;Ticket* previous;
 Scope(Client& c,const EQ::ItemInstance& item,Corpse& corpse):ticket{&c,&item,item.GetSerialNumber(),&corpse},previous(active){active=&ticket;}
 ~Scope(){active=previous;}
};
inline void Saved(Client& c,const EQ::ItemInstance& item,bool acknowledged) {
 auto* t=active;
 if(!t||t->consumed||!acknowledged||t->client!=&c||t->item!=&item||t->serial!=item.GetSerialNumber()||(item.GetID()!=17285&&item.GetID()!=17312)||
  !RuleB(SodProgression,EnableOceangreenSlice)||!zone||
  !t->corpse->IsNPCCorpse())return;
 if(item.GetID()==17312) {
  if(zone->GetZoneID()!=466||zone->GetInstanceID()!=0||zone->GetInstanceVersion()!=0||t->corpse->GetNPCTypeID()!=466057||!c.IsTaskActivityActive(920182,0))return;
  t->consumed=true;c.UpdateTaskActivity(920182,0,1);return;
 }
 if(zone->GetZoneID()!=468||zone->GetInstanceVersion()!=1||!zone->GetInstanceID()||!c.IsTaskActivityActive(920183,0))return;
 auto role=SodContent::Role(content_db,920183,"Kegrak");if(!role||t->corpse->GetNPCTypeID()!=*role)return;
 if(!SodProgression::MissionEntry(database,content_db,c.CharacterID(),c.GetSharedTaskId(),468,1,zone->GetInstanceID(),false))return;
 t->consumed=true;c.UpdateTaskActivity(920183,0,1);
}
}
