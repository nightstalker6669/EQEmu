#pragma once
#include "../database.h"
#include <mysqld_error.h>
#include <stdexcept>
#include <vector>
namespace EQEmu::SharedCore::PendingOperations {
enum class PendingState { Clear,Pending,Unavailable };
using Bytes=std::vector<uint8_t>;
inline uint64_t Number(const char* value,uint64_t max){if(!value)throw std::invalid_argument("Missing pending field");std::string s(value);if(s.empty()||s[0]=='-')throw std::invalid_argument("Invalid pending number");size_t at=0;auto n=std::stoull(s,&at);if(at!=s.size()||n>max)throw std::invalid_argument("Pending number range");return n;}
inline Bytes Unhex(const char* input){if(!input)throw std::invalid_argument("Missing pending key");std::string s(input);if(s.size()%2||s.size()>4*1024*1024)throw std::invalid_argument("Pending key bound");auto nibble=[](char c)->uint8_t{if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;throw std::invalid_argument("Pending key hex");};Bytes out;for(size_t n=0;n<s.size();n+=2)out.push_back((nibble(s[n])<<4)|nibble(s[n+1]));return out;}
    inline PendingState InspectPending(Database& db,uint32_t actor,bool lock)noexcept {
        try {
            auto schema=db.QueryDatabase(std::string("SELECT COUNT(*),SUM(engine='InnoDB') FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('housing_house_inventory_pending','housing_house_inventory_receipts')"),false);
            if(!schema.Success()||schema.RowCount()!=1)return PendingState::Unavailable;
            auto count=Number(schema.begin()[0],2);
            if(!count){
                auto p=db.QueryDatabase(std::string("SELECT request_key,inventory_epoch FROM housing_house_inventory_pending WHERE 1=0"),false);
                auto r=db.QueryDatabase(std::string("SELECT request_key,character_id,inventory_epoch,applied FROM housing_house_inventory_receipts WHERE 1=0"),false);
                return !p.Success()&&!r.Success()&&p.ErrorNumber()==ER_NO_SUCH_TABLE&&r.ErrorNumber()==ER_NO_SUCH_TABLE?PendingState::Clear:PendingState::Unavailable;
            }
            if(count!=2||Number(schema.begin()[1],2)!=2)return PendingState::Unavailable;
            auto rows=db.QueryDatabase("SELECT p.inventory_epoch,HEX(p.request_key),r.character_id,r.account_id,r.inventory_epoch,r.applied FROM housing_house_inventory_pending p LEFT JOIN housing_house_inventory_receipts r ON r.request_key=p.request_key WHERE p.character_id="+std::to_string(actor)+(lock?" FOR UPDATE":""),false);
            if(!rows.Success()||rows.RowCount()>1)return PendingState::Unavailable;
            if(!rows.RowCount()){
                auto orphan=db.QueryDatabase("SELECT request_key FROM housing_house_inventory_receipts WHERE character_id="+std::to_string(actor)+" AND applied=0"+(lock?" FOR UPDATE":""),false);
                if(!orphan.Success()||orphan.RowCount())return PendingState::Unavailable;
                auto dangling=db.QueryDatabase("SELECT h.house_id FROM housing_house_instances h JOIN housing_house_inventory_receipts r17 ON r17.request_key=h.pending_request_key LEFT JOIN housing_house_receipt_effects e7 ON e7.request_key=h.pending_request_key AND e7.house_id=h.house_id LEFT JOIN housing_live_item_receipts r7 ON r7.request_key=e7.request_key WHERE r17.character_id="+std::to_string(actor)+" AND NOT (r7.character_id IS NOT NULL AND r7.character_id="+std::to_string(actor)+" AND r7.completed=1 AND r7.applied=0)"+(lock?" FOR UPDATE":""),false);
                return dangling.Success()&&!dangling.RowCount()?PendingState::Clear:PendingState::Unavailable;
            }
            auto row=rows.begin();
            auto epoch=Number(row[0],UINT64_MAX);
            auto key=Unhex(row[1]);
            if(!epoch||key.empty()||key.size()>96||Number(row[2],UINT32_MAX)!=actor||!Number(row[3],UINT32_MAX)||Number(row[4],UINT64_MAX)!=epoch||Number(row[5],1)!=0)return PendingState::Unavailable;
            return PendingState::Pending;
        }catch(...){
            return PendingState::Unavailable;
        }
    }
}
