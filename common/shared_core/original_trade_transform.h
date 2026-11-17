#pragma once
#include "original_trade_recipe.h"
#include <stdexcept>
#include <string>
// Internal semantic context only. No World packet/opcode encoding is defined.
namespace EQEmu::SharedCore::OriginalTradeTransform {
namespace T=OriginalTradeRecipe;
inline constexpr uint32_t Kind=3; // Database reservation 20261116; 1/2 unchanged.
enum class Output { ReturnOriginal, CreateBound, ConsumeOnly, ConsumeOnAdmission };
struct Context {
 uint32_t character=0,account=0,task=0,activity=0,accepted_time=0,model=0;
 uint64_t occurrence=0; // Actual PERSONAL occurrence or actual SHARED run.
 bool shared=false;
 uint32_t npc_type=0,npc_entity=0,npc_spawn=0,zone=0,version=0,instance=0;
 uint32_t input_item=0,output_item=0,primary=0;
 Output output=Output::ReturnOriginal;
 T::Offers offers{};
 // Native attribution of each original root; requested skull1 + ordinary
 // parallel tails0 are ONE original request. Unrelated roots request zero.
 std::array<uint32_t,T::OfferCount> activities{},requested{};
 uint32_t before_tail=0;
};
enum class DecisionState { Pending, PreparedPersonal, Known };
struct Decision {
 DecisionState state=DecisionState::Pending;
 std::array<uint32_t,T::OfferCount> accepted{};
 bool counters_present=false;
 uint32_t tail_before=0,tail_target=0,primary_before=0,primary_target=0;
 uint64_t admitted_run=0;
 uint32_t admitted_accepted_time=0;
};
inline bool Admission(const Context& c) {return c.output==Output::ConsumeOnAdmission;}
inline bool MessageGiver(uint32_t npc,uint32_t spawn) {
 return (npc==452079&&spawn==129019)||(npc==452108&&spawn==129109)||(npc==452121&&spawn==129149)||
  (npc==452133&&spawn==129172)||(npc==452147&&spawn==129209)||(npc==452150&&spawn==129220);
}
inline uint32_t MessageTask(uint32_t item) {
 switch(item){case 88318:return 920200;case 88310:return 920201;case 199900:return 920202;case 199901:return 920203;default:return 0;}
}
inline bool Valid(const Context& c) {
 if(!c.character||!c.account||(!Admission(c)&&(!c.occurrence||!c.accepted_time))||!c.model||!c.npc_entity||c.zone!=452||c.version||c.instance||c.primary>=T::OfferCount||!T::Valid(c.offers,{}))return false;
 const auto& p=c.offers[c.primary];
 if(!p.present||p.item_id!=c.input_item||!c.requested[c.primary]||c.requested[c.primary]>T::Available(p)||c.activities[c.primary]!=c.activity)return false;
 if(c.activity!=0&&(p.stackable||c.requested[c.primary]!=1))return false;
 if(Admission(c)) {
  if(!c.shared||c.occurrence||c.accepted_time||c.activity||c.output_item||c.before_tail||!MessageGiver(c.npc_type,c.npc_spawn)||!MessageTask(c.input_item)||MessageTask(c.input_item)!=c.task||c.requested[c.primary]!=1)return false;
  for(size_t i=0;i<T::OfferCount;++i)if(i!=c.primary&&c.requested[i])return false;
  return true;
 }else if(c.task==920197&&c.activity==0) {
  if(!c.shared||c.npc_type!=452108||c.npc_spawn!=129109||c.input_item!=88325||c.output_item||c.output!=Output::ConsumeOnly||c.before_tail>4)return false;
 }else if(c.task==920197&&c.activity==1) {
  if(!c.shared||c.npc_type!=452108||c.npc_spawn!=129109||c.input_item!=88326||c.output_item!=88326||c.output!=Output::ReturnOriginal||c.before_tail>4)return false;
 }else if(c.task==920207&&c.activity==1) {
  if(c.shared||c.npc_type!=452150||c.npc_spawn!=129220||c.input_item!=17381||c.output_item!=17381||c.output!=Output::ReturnOriginal)return false;
 }else if(c.task==920207&&c.activity==7) {
  if(c.shared||c.npc_type!=452133||c.npc_spawn!=129172||c.input_item!=50004||c.output_item!=37763||c.output!=Output::CreateBound)return false;
 }else if(c.task==920207&&c.activity==9) {
  // Exact approved dynamic clock actor. Spawn0 is its required identity, not a wildcard.
  if(c.shared||c.npc_type!=452918||c.npc_spawn!=0||c.input_item!=37763||c.output_item!=17388||c.output!=Output::CreateBound)return false;
 }else return false;
 uint64_t tails=c.activity==0?c.requested[c.primary]:0;
 for(size_t i=0;i<T::OfferCount;++i)if(i!=c.primary&&c.requested[i]) {
  if(!c.shared||!c.offers[i].present||c.offers[i].item_id!=88325||c.activities[i]!=0||c.requested[i]>T::Available(c.offers[i]))return false;
  tails+=c.requested[i];
 }
 return tails<=4; // World decides actual remaining native units at its commit.
}
inline bool Valid(const Context& c,const Decision& d) {
 if(!Valid(c)||(d.state!=DecisionState::Known&&!(d.state==DecisionState::PreparedPersonal&&!c.shared)))return false;
 if(Admission(c)) {
  if(d.state!=DecisionState::Known||d.counters_present||d.tail_before||d.tail_target||d.primary_before||d.primary_target)return false;
  uint64_t units=0;for(size_t i=0;i<T::OfferCount;++i){if(d.accepted[i]>c.requested[i])return false;units+=d.accepted[i];}
  return units==1?(d.accepted[c.primary]==1&&d.admitted_run&&d.admitted_accepted_time):(!units&&!d.admitted_run&&!d.admitted_accepted_time);
 }
 if(d.admitted_run||d.admitted_accepted_time)return false;
 uint64_t tails=0,skulls=0;
 for(size_t i=0;i<T::OfferCount;++i){if(d.accepted[i]>c.requested[i])return false;if(c.shared){if(c.activities[i]==0)tails+=d.accepted[i];else if(c.activities[i]==1)skulls+=d.accepted[i];else if(d.accepted[i])return false;}}
 // primary_before/target always denote the shared SKULL goal1, even when a
 // tails-only context's selected primary physical root belongs to goal0.
 if(c.shared){
  if(!d.counters_present)return !tails&&!skulls&&!d.tail_before&&!d.tail_target&&!d.primary_before&&!d.primary_target;
  return d.tail_before<=4&&d.tail_target<=4&&d.tail_target==d.tail_before+tails&&d.primary_before<=1&&d.primary_target<=1&&d.primary_target==d.primary_before+skulls;
 }
 if(!d.counters_present)return false;
 uint64_t total=0;for(auto n:d.accepted)total+=n;
 return d.accepted[c.primary]==1&&total==1&&d.primary_before==0&&d.primary_target==1&&!d.tail_before&&!d.tail_target;
}
inline T::DebitPlan Debit(const Context& c,const Decision& d) {
 if(!Valid(c,d))throw std::runtime_error("Original transform decision differs");
 T::DebitPlan out;out.units=d.accepted;
 if(c.output==Output::ReturnOriginal)out.units[c.primary]=0;
 if(!T::Valid(c.offers,out))throw std::runtime_error("Original transform root debit differs");return out;
}
// Fixed bounded internal tuple; uint64 never crosses floating-point/JSON. The
// task/activity pair identifies the route, no new route/item ID is allocated.
inline void W(std::string& b,uint64_t n,unsigned width){for(unsigned i=0;i<width;++i)b+=char(n>>(8*i));}
inline std::string Encode(const Context& c) {
 if(!Valid(c))throw std::runtime_error("Original transform context unbound");std::string b="SOTX1";
 auto w=[&](uint64_t v,unsigned n=4){W(b,v,n);};
 w(c.character);w(c.account);w(c.task);w(c.activity);w(c.accepted_time);w(c.model);w(c.occurrence,8);w(c.shared,1);
 w(c.npc_type);w(c.npc_entity);w(c.npc_spawn);w(c.zone);w(c.version);w(c.instance);w(c.input_item);w(c.output_item);w(c.primary);w(uint32_t(c.output));w(c.before_tail);
 for(size_t i=0;i<T::OfferCount;++i){const auto& o=c.offers[i];w(o.present,1);w(uint16_t(o.slot),2);w(uint32_t(o.guid));w(o.item_id);w(uint16_t(o.charges),2);w(o.stackable,1);w(c.activities[i]);w(c.requested[i]);}return b;
}
inline Context Decode(const std::string& b) {
 if(b.size()>1024||b.substr(0,5)!="SOTX1")throw std::runtime_error("Original transform context encoding differs");size_t at=5;
 auto n=[&](unsigned width=4)->uint64_t{if(at+width>b.size())throw std::runtime_error("Original transform context truncated");uint64_t out=0;for(unsigned i=0;i<width;++i)out|=uint64_t(uint8_t(b[at++]))<<(i*8);return out;};
 Context c;c.character=n();c.account=n();c.task=n();c.activity=n();c.accepted_time=n();c.model=n();c.occurrence=n(8);auto shared=n(1);if(shared>1)throw std::runtime_error("Original transform shared flag differs");c.shared=shared;
 c.npc_type=n();c.npc_entity=n();c.npc_spawn=n();c.zone=n();c.version=n();c.instance=n();c.input_item=n();c.output_item=n();c.primary=n();c.output=Output(n());c.before_tail=n();
 for(size_t i=0;i<T::OfferCount;++i){auto& o=c.offers[i];auto present=n(1);o.slot=int16_t(n(2));o.guid=int32_t(n());o.item_id=n();o.charges=int16_t(n(2));auto stack=n(1);if(present>1||stack>1)throw std::runtime_error("Original transform offer flag differs");o.present=present;o.stackable=stack;c.activities[i]=n();c.requested[i]=n();}
 if(at!=b.size()||Encode(c)!=b)throw std::runtime_error("Original transform context noncanonical");return c;
}
}
