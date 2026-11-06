#pragma once
#include <cstdint>
#include <cstring>
namespace FungalItemWire {
constexpr uint32_t Task=920143,Magic=0x46554e31;
// Feature-private optional suffix on the existing native shared-activity packet.
// Original prefix, opcode and all other tasks remain unchanged.
#pragma pack(push,1)
struct Trailer { uint32_t magic=Magic,version=1;uint64_t run=0,outcome=0;
 uint32_t accepted=0,recipient=0; };
#pragma pack(pop)
inline bool Selected(uint32_t task,uint32_t activity)noexcept {
 return task==Task&&(activity==5||activity==10);
}
inline bool PureHail(uint32_t task,uint32_t activity)noexcept {
 return task==Task&&(activity==0||activity==1||activity==6);
}
inline bool Correlated(uint32_t task,uint32_t activity)noexcept {
 return Selected(task,activity)||PureHail(task,activity);
}
inline bool ReadHeader(const void* bytes,uint32_t size,uint32_t prefix,Trailer& out)noexcept {
 if(size!=prefix+sizeof(Trailer))return false;
 std::memcpy(&out,static_cast<const unsigned char*>(bytes)+prefix,sizeof(out));
 return out.magic==Magic&&out.version==1&&out.run&&out.accepted&&out.recipient;
}
inline bool Read(const void* bytes,uint32_t size,uint32_t prefix,Trailer& out)noexcept {
 return ReadHeader(bytes,size,prefix,out)&&out.outcome;
}
inline void Write(void* bytes,uint32_t prefix,const Trailer& value)noexcept {
 std::memcpy(static_cast<unsigned char*>(bytes)+prefix,&value,sizeof(value));
}
}
