#pragma once
#include "common/shared_tasks.h"
#include "common/sod_reward_quote.h"
#include <cstring>
namespace SodSharedActivity {
struct Trailer { uint32_t magic=0x534f4441,version=1;uint64_t run=0; };
inline void Write(unsigned char* bytes,uint64_t run) {
 Trailer t;t.run=run;std::memcpy(bytes+sizeof(ServerSharedTaskActivityUpdate_Struct),&t,sizeof(t));
}
inline bool Read(const unsigned char* bytes,uint32_t size,uint64_t& run) {
 run=0;if(size!=sizeof(ServerSharedTaskActivityUpdate_Struct)+sizeof(Trailer))return false;
 Trailer t;std::memcpy(&t,bytes+sizeof(ServerSharedTaskActivityUpdate_Struct),sizeof(t));
 if(t.magic!=0x534f4441||t.version!=1||!t.run)return false;run=t.run;return true;
}
}
