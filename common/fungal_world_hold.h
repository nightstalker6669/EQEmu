#pragma once
#include <cstdint>
namespace FungalInventory {
class WorldCustody;
class WorldHold final {
 friend class WorldCustody;
 explicit WorldHold(uint64_t run):run_(run){}
 uint64_t run_;WorldHold* next_=nullptr;
 inline static thread_local WorldHold* first_=nullptr;
 void Attach()noexcept{next_=first_;first_=this;}
public:
 ~WorldHold()=default;
 WorldHold(const WorldHold&)=delete;
 WorldHold& operator=(const WorldHold&)=delete;
 // Source's existing native run update/save entry must call this before ANY
 // native mutation/full snapshot save, including non-Fungal stages in this run.
 // Unknown runs remain held until authoritative owner reload/process teardown.
 static bool Allows(uint64_t run)noexcept{for(auto* p=first_;p;p=p->next_)if(p->run_==run)return false;return true;}
};
}
