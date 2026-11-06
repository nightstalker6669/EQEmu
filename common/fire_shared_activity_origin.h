// OFFLINE SOURCE PROPOSAL ONLY. Fire170, matched World/Zone peers, no activation.
#pragma once
#include "fire_shared_correlation.h"
namespace FireSharedActivityOrigin {
constexpr uint32_t Task=920170,Magic=0x46495241,Version=1;
struct Trailer { uint32_t magic=Magic,version=Version;uint64_t run=0; };
inline bool Read(const unsigned char* bytes,uint32_t size,uint32_t task,uint64_t& run) {
 run=0;
 if(task!=Task) return size==sizeof(ServerSharedTaskActivityUpdate_Struct);
 if(size!=sizeof(ServerSharedTaskActivityUpdate_Struct)+sizeof(Trailer)) return false;
 Trailer t;
 std::memcpy(&t,bytes+sizeof(ServerSharedTaskActivityUpdate_Struct),sizeof(t));
 if(t.magic!=Magic || t.version!=Version || !t.run) return false;
 run=t.run;return true;
}
inline void Write(unsigned char* bytes,uint64_t run) {
 Trailer t;t.run=run;
 std::memcpy(bytes+sizeof(ServerSharedTaskActivityUpdate_Struct),&t,sizeof(t));
}
} // namespace FireSharedActivityOrigin
