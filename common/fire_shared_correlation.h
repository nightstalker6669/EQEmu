// OFFLINE SOURCE PROPOSAL ONLY. Matched World/Zone peers required; not activated.
#pragma once
#include "shared_tasks.h"
#include "database.h"
#include "dynamic_zone_base.h"
#include "fmt/format.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <optional>
#include <string>
#include <vector>
namespace FireSharedCorrelation {
constexpr uint32_t Task=920170,SourceTask=920169,Magic=0x46495245,Version=1,MaxMembers=6;
enum Phase : uint32_t { Request=1,Admitted=2,Ready=3,Held=4 };
// Optional trailer only: ordinary ServerSharedTaskRequest_Struct is unchanged.
struct Trailer {
 uint32_t magic=Magic,version=Version,phase=Request,count=0;
 uint64_t source_occurrence=0,pending_reward=0,run=0;
 uint32_t dz=0,instance=0;
 uint32_t members[MaxMembers]{};
};
enum class Kind { Ordinary,Fire,Invalid };
inline Kind Decode(const unsigned char* bytes,uint32_t size,Trailer& out) {
 if(size==sizeof(ServerSharedTaskRequest_Struct)) return Kind::Ordinary;
 if(size!=sizeof(ServerSharedTaskRequest_Struct)+sizeof(Trailer)) return Kind::Invalid;
 std::memcpy(&out,bytes+sizeof(ServerSharedTaskRequest_Struct),sizeof(out));
 if(out.magic!=Magic || out.version!=Version || out.phase<Request || out.phase>Held ||
  !out.source_occurrence || !out.pending_reward || out.count>MaxMembers) return Kind::Invalid;
 return Kind::Fire;
}
inline bool RequestShape(const Trailer& t) {
 if(t.phase!=Request || t.count || t.run || t.dz || t.instance) return false;
 for(auto id:t.members) if(id) return false;
 return true;
}
inline bool DeliveredSource(Database& db,uint32_t character,const Trailer& t) {
 auto r=db.QueryDatabase(fmt::format(
  "SELECT 1 FROM character_task_reward_selections s WHERE s.character_id={} AND s.task_id=920169 "
  "AND s.source_instance_id={} AND s.pending_reward_id={} AND s.reward_set_id=920169 AND s.status=1 "
  "AND EXISTS(SELECT 1 FROM character_task_rewards e WHERE e.character_id=s.character_id "
  "AND e.pending_reward_id=s.pending_reward_id AND e.reward_id=3839201691 AND e.status=1)",
  character,t.source_occurrence,t.pending_reward));
 return r.Success() && r.RowCount()==1; // validate a received genuine origin, never reconstruct one
}
inline std::optional<uint64_t> Number(const char* text) {
 if(!text) return std::nullopt;
 const std::string s(text);uint64_t n=0;
 const auto r=std::from_chars(s.data(),s.data()+s.size(),n);
 if(r.ec!=std::errc{} || r.ptr!=s.data()+s.size() || !n) return std::nullopt;
 return n;
}
inline std::optional<std::vector<uint32_t>> Roster(const Trailer& t,uint32_t requester) {
 if(!t.count || t.count>MaxMembers) return std::nullopt;
 std::vector<uint32_t> ids(t.members,t.members+t.count);
 if(std::any_of(ids.begin(),ids.end(),[](auto id){return id==0;})) return std::nullopt;
 std::sort(ids.begin(),ids.end());
 if(std::adjacent_find(ids.begin(),ids.end())!=ids.end() ||
  !std::binary_search(ids.begin(),ids.end(),requester)) return std::nullopt;
 for(uint32_t i=t.count;i<MaxMembers;++i) if(t.members[i]) return std::nullopt;
 return ids;
}
inline bool ExactRoster(Database& db,const std::string& sql,const std::vector<uint32_t>& expected) {
 auto r=db.QueryDatabase(sql);
 if(!r.Success() || r.RowCount()!=expected.size()) return false;
 std::vector<uint32_t> actual;
 for(auto row:r) {auto id=Number(row[0]);if(!id || *id>UINT32_MAX) return false;actual.push_back(*id);}
 std::sort(actual.begin(),actual.end());return actual==expected;
}
inline bool HeaderAndRoster(Database& db,const ServerSharedTaskRequest_Struct& p,const Trailer& t) {
 auto ids=Roster(t,p.requested_character_id);
 if(!ids || !t.run || p.requested_task_id!=Task || !p.accept_time) return false;
 auto h=db.QueryDatabase(fmt::format(
  "SELECT 1 FROM shared_tasks WHERE id={} AND task_id={} AND accepted_time=FROM_UNIXTIME({}) "
  "AND completion_time IS NULL AND (expire_time IS NULL OR expire_time>NOW())",
  t.run,Task,p.accept_time));
 return h.Success() && h.RowCount()==1 && ExactRoster(db,fmt::format(
  "SELECT character_id FROM shared_task_members WHERE shared_task_id={}",t.run),*ids);
}
inline bool PersistedMissionReady(Database& db,const ServerSharedTaskRequest_Struct& p,const Trailer& t) {
 auto ids=Roster(t,p.requested_character_id);
 if(!ids || !t.dz || !t.instance || t.instance>UINT16_MAX || !HeaderAndRoster(db,p,t)) return false;
 auto h=db.QueryDatabase(fmt::format(
  "SELECT 1 FROM shared_task_dynamic_zones m JOIN dynamic_zones d ON d.id=m.dynamic_zone_id "
  "JOIN instance_list i ON i.id=d.instance_id WHERE m.shared_task_id={} AND d.id={} "
  "AND d.instance_id={} AND d.type={} AND i.zone=490 AND i.version=1 "
  "AND (i.never_expires=1 OR i.expire_at>UNIX_TIMESTAMP()) "
  "AND (SELECT COUNT(*) FROM shared_task_dynamic_zones WHERE shared_task_id={})=1",
  t.run,t.dz,t.instance,static_cast<int>(DynamicZoneType::Mission),t.run));
 return h.Success() && h.RowCount()==1 && ExactRoster(db,fmt::format(
  "SELECT character_id FROM dynamic_zone_members WHERE dynamic_zone_id={}",t.dz),*ids) &&
  ExactRoster(db,fmt::format("SELECT charid FROM instance_list_player WHERE id={}",t.instance),*ids);
}
} // namespace FireSharedCorrelation
