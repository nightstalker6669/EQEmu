#pragma once
#include "../database.h"
namespace EQEmu::SharedCore {
inline bool RegisteredLiveProcess(Database& db,const std::string& key) noexcept {
 try {
  if(key.empty()||key.size()>96)return false;
  for(unsigned char c:key)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==':'||c=='_'||c=='-'||c=='.'))return false;
  auto engine=db.QueryDatabase(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_inventory_processes'"),false);
  if(!engine.Success()||engine.RowCount()!=1||!engine.begin()[0]||std::string(engine.begin()[0])!="InnoDB")return false;
  auto rows=db.QueryDatabase("SELECT reaped FROM housing_inventory_processes WHERE process_key='"+db.Escape(key)+"' FOR UPDATE",false);
  return rows.Success()&&rows.RowCount()==1&&rows.begin()[0]&&std::string(rows.begin()[0])=="0";
 }catch(...){return false;}
}
}
