#pragma once
#include "database.h"
#include <map>
#include <stdexcept>
#include <string>
namespace FungalCustodySQL {
inline uint64_t Number(const char* p){if(!p)throw std::runtime_error("Fungal missing identity");std::string s=p;size_t n=0;auto v=std::stoull(s,&n);if(n!=s.size())throw std::runtime_error("Fungal malformed identity");return v;}
inline auto Query(Database& db,const std::string& sql){if(db.GetStatus()!=DBcore::Connected)throw std::runtime_error("Fungal disconnected");auto r=db.QueryDatabase(sql,false);if(!r.Success())throw std::runtime_error("Fungal strict query failed");return r;}
struct Activity {int count=0;uint64_t updated=0,completed=0;};
inline std::map<int,Activity> LockShared(Database& db,uint64_t run,uint32_t accepted,uint32_t member,uint32_t activity,bool allow_complete) {
 const bool pure=activity==0||activity==1||activity==6;
 if(!run||!accepted||!member||(!pure&&activity!=5&&activity!=10))throw std::runtime_error("Fungal unsupported run");
 auto engines=Query(db,pure?"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()AND engine='InnoDB'AND table_name IN('shared_tasks','shared_task_members','shared_task_activity_state')":"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()AND engine='InnoDB'AND table_name IN('shared_tasks','shared_task_members','shared_task_activity_state','fungal_item_operations')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=(pure?3u:4u))throw std::runtime_error("Fungal shared foundation differs");
 auto task=Query(db,"SELECT id FROM shared_tasks WHERE id="+std::to_string(run)+" AND task_id=920143 AND accepted_time=FROM_UNIXTIME("+std::to_string(accepted)+") AND completion_time IS NULL AND(expire_time IS NULL OR expire_time>NOW()) FOR UPDATE");if(task.RowCount()!=1)throw std::runtime_error("Fungal native run changed");
 auto membership=Query(db,"SELECT character_id FROM shared_task_members WHERE shared_task_id="+std::to_string(run)+" AND character_id="+std::to_string(member)+" FOR UPDATE");if(membership.RowCount()!=1)throw std::runtime_error("Fungal native member changed");
 auto rows=Query(db,"SELECT activity_id,done_count,COALESCE(UNIX_TIMESTAMP(updated_time),0),COALESCE(UNIX_TIMESTAMP(completed_time),0) FROM shared_task_activity_state WHERE shared_task_id="+std::to_string(run)+" ORDER BY activity_id FOR UPDATE");std::map<int,Activity> snapshot;
 for(auto r=rows.begin();r!=rows.end();++r){int id=Number(r[0]);if(!snapshot.emplace(id,Activity{int(Number(r[1])),Number(r[2]),Number(r[3])}).second)throw std::runtime_error("Fungal duplicate stage");}
 auto selected=snapshot.find(activity);if(selected==snapshot.end()||(!allow_complete&&(selected->second.count||selected->second.completed)))throw std::runtime_error("Fungal selected stage differs");
 if(activity==10)for(auto pair:{std::pair<int,int>{7,4},{8,3},{9,2}}){auto at=snapshot.find(pair.first);if(at==snapshot.end()||at->second.count<pair.second||!at->second.completed)throw std::runtime_error("Fungal original deliveries incomplete");}
 return snapshot;
}
inline void LockPersonal(Database& db,uint64_t occurrence,uint32_t accepted,uint32_t character,int slot) {
 auto engines=Query(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()AND engine='InnoDB'AND table_name IN('character_task_reward_instances','character_tasks','character_activities')");if(engines.RowCount()!=1||Number(engines.begin()[0])!=3)throw std::runtime_error("Fungal personal foundation differs");
 auto identity=Query(db,"SELECT occurrence_id FROM character_task_reward_instances WHERE character_id="+std::to_string(character)+" AND task_id=920141 AND occurrence_id="+std::to_string(occurrence)+" AND accepted_time="+std::to_string(accepted)+" FOR UPDATE");if(identity.RowCount()!=1)throw std::runtime_error("Fungal personal occurrence changed");
 auto task=Query(db,"SELECT taskid FROM character_tasks WHERE charid="+std::to_string(character)+" AND taskid=920141 AND acceptedtime="+std::to_string(accepted)+" AND slot="+std::to_string(slot)+" AND type=2 AND was_rewarded=0 FOR UPDATE");if(task.RowCount()!=1)throw std::runtime_error("Fungal personal acceptance changed");
 auto stage=Query(db,"SELECT activityid FROM character_activities WHERE charid="+std::to_string(character)+" AND taskid=920141 AND activityid=7 AND donecount=0 AND completed=0 FOR UPDATE");if(stage.RowCount()!=1)throw std::runtime_error("Fungal personal stage changed");
}
}
