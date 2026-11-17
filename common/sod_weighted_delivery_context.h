#pragma once
#include "common/shared_core/weighted_personal_delivery.h"
#include "common/sod_weighted_delivery_recovery.h"
#include <stdexcept>

// Canonical semantic original-offer context. Contains NO physical inventory
// tree/return blob. uint64 occurrences/revisions remain integers end-to-end.
namespace SodWeightedDeliveryContext {
namespace W=EQEmu::SharedCore::WeightedPersonalDelivery;
namespace T=EQEmu::SharedCore::OriginalTradeRecipe;
struct Npc {uint32_t type=0,entity=0,spawn=0,zone=0,version=0,instance=0;};
struct Context {
 W::Occurrence occurrence;
 W::Parameters parameters;
 T::Offers offers;
 W::Plan plan;
 Npc npc;
 uint32_t recipe_model=0;
 uint64_t observed_at=0;
};
inline void Put(std::string& out,uint64_t value,unsigned width) {
 for(unsigned n=0;n<width;++n)out.push_back(static_cast<char>((value>>(n*8))&255));
}
struct Reader {
 const std::string& bytes;size_t at=0;
 uint64_t Get(unsigned width) {
  if(width>8 || at>bytes.size() || width>bytes.size()-at)throw std::runtime_error("Food semantic context truncated");
  uint64_t out=0;for(unsigned n=0;n<width;++n)out|=uint64_t(static_cast<unsigned char>(bytes[at++]))<<(n*8);
  return out;
 }
};
inline SodWeightedDeliveryRecovery::Identity Identity(const Context& c) {
 return {c.occurrence.character_id,c.occurrence.task_id,c.occurrence.activity_id,
  c.occurrence.accepted_time,c.recipe_model,c.occurrence.occurrence_id};
}
inline bool SamePlan(const W::Plan& a,const W::Plan& b) {
 return a.debit.units==b.debit.units && a.accepted_pounds==b.accepted_pounds &&
  a.credit_units==b.credit_units && a.progress_after==b.progress_after &&
  a.carry_after==b.carry_after && a.carry_expires_after==b.carry_expires_after;
}
inline bool Valid(const Context& c) {
 if(c.occurrence.task_id!=920198 || c.occurrence.activity_id!=1 ||
  !c.recipe_model || !c.npc.type || !c.npc.entity || c.npc.zone!=452 ||
  c.npc.version || c.npc.instance || c.parameters.pounds_per_unit!=10 ||
  c.parameters.goal_units!=10 || c.parameters.roles[0].pounds!=7 ||
  c.parameters.roles[1].pounds!=4 || c.parameters.roles[2].pounds!=2 ||
  c.parameters.stack_rule<W::StackRule::ReturnStacked ||
  c.parameters.stack_rule>W::StackRule::AcceptStacked ||
  c.parameters.excess_rule!=W::ExcessRule::ConsumeEligibleAndDiscardAtCompletion ||
  c.parameters.carry_clock<W::CarryClock::RefreshOnAccept ||
  c.parameters.carry_clock>W::CarryClock::PreserveUnexpired)return false;
 W::Plan plan;
 return W::Prepare(c.parameters,c.occurrence,c.offers,c.observed_at,plan)==W::Status::Ready && SamePlan(plan,c.plan);
}
inline std::string Encode(const Context& c) {
 if(!Valid(c))throw std::runtime_error("Food semantic context is unbound or invalid");
 std::string out="FSWT";Put(out,1,4);
 const auto& o=c.occurrence;
 Put(out,o.character_id,4);Put(out,o.occurrence_id,8);Put(out,o.accepted_time,4);
 Put(out,o.task_id,4);Put(out,o.activity_id,4);Put(out,o.progress,4);
 Put(out,o.carry_pounds,4);Put(out,o.carry_expires_at,8);Put(out,o.revision,8);
 const auto& p=c.parameters;
 Put(out,p.task_id,4);Put(out,p.activity_id,4);Put(out,p.pounds_per_unit,4);Put(out,p.goal_units,4);
 for(const auto& role:p.roles){Put(out,role.item_id,4);Put(out,role.pounds,4);}
 Put(out,static_cast<uint32_t>(p.stack_rule),4);Put(out,static_cast<uint32_t>(p.excess_rule),4);
 Put(out,static_cast<uint32_t>(p.carry_clock),4);Put(out,p.carry_lifetime_seconds,8);
 for(const auto& a:c.offers) {
  Put(out,a.present?1:0,1);Put(out,static_cast<uint16_t>(a.slot),2);
  Put(out,static_cast<uint32_t>(a.guid),4);Put(out,a.item_id,4);
  Put(out,static_cast<uint16_t>(a.charges),2);Put(out,a.stackable?1:0,1);
 }
 for(const auto units:c.plan.debit.units)Put(out,units,4);
 Put(out,c.plan.accepted_pounds,4);Put(out,c.plan.credit_units,4);Put(out,c.plan.progress_after,4);
 Put(out,c.plan.carry_after,4);Put(out,c.plan.carry_expires_after,8);
 Put(out,c.npc.type,4);Put(out,c.npc.entity,4);Put(out,c.npc.spawn,4);
 Put(out,c.npc.zone,4);Put(out,c.npc.version,4);Put(out,c.npc.instance,4);
 Put(out,c.recipe_model,4);Put(out,c.observed_at,8);
 return out;
}
inline Context Decode(const std::string& bytes) {
 if(bytes.size()<8 || bytes.compare(0,4,"FSWT")!=0)throw std::runtime_error("Food semantic context magic differs");
 Reader r{bytes,4};if(r.Get(4)!=1)throw std::runtime_error("Food semantic context version differs");
 Context c;auto& o=c.occurrence;
 o.character_id=uint32_t(r.Get(4));o.occurrence_id=r.Get(8);o.accepted_time=uint32_t(r.Get(4));
 o.task_id=uint32_t(r.Get(4));o.activity_id=uint32_t(r.Get(4));o.progress=uint32_t(r.Get(4));
 o.carry_pounds=uint32_t(r.Get(4));o.carry_expires_at=r.Get(8);o.revision=r.Get(8);
 auto& p=c.parameters;
 p.task_id=uint32_t(r.Get(4));p.activity_id=uint32_t(r.Get(4));p.pounds_per_unit=uint32_t(r.Get(4));p.goal_units=uint32_t(r.Get(4));
 for(auto& role:p.roles){role.item_id=uint32_t(r.Get(4));role.pounds=uint32_t(r.Get(4));}
 p.stack_rule=static_cast<W::StackRule>(r.Get(4));p.excess_rule=static_cast<W::ExcessRule>(r.Get(4));
 p.carry_clock=static_cast<W::CarryClock>(r.Get(4));p.carry_lifetime_seconds=r.Get(8);
 for(auto& a:c.offers) {
  const auto present=r.Get(1);a.slot=static_cast<int16_t>(r.Get(2));a.guid=static_cast<int32_t>(r.Get(4));
  a.item_id=uint32_t(r.Get(4));a.charges=static_cast<int16_t>(r.Get(2));const auto stack=r.Get(1);
  if(present>1 || stack>1)throw std::runtime_error("Food semantic context boolean differs");
  a.present=present!=0;a.stackable=stack!=0;
 }
 for(auto& units:c.plan.debit.units)units=uint32_t(r.Get(4));
 c.plan.accepted_pounds=uint32_t(r.Get(4));c.plan.credit_units=uint32_t(r.Get(4));c.plan.progress_after=uint32_t(r.Get(4));
 c.plan.carry_after=uint32_t(r.Get(4));c.plan.carry_expires_after=r.Get(8);
 c.npc.type=uint32_t(r.Get(4));c.npc.entity=uint32_t(r.Get(4));c.npc.spawn=uint32_t(r.Get(4));
 c.npc.zone=uint32_t(r.Get(4));c.npc.version=uint32_t(r.Get(4));c.npc.instance=uint32_t(r.Get(4));
 c.recipe_model=uint32_t(r.Get(4));c.observed_at=r.Get(8);
 if(r.at!=bytes.size() || Encode(c)!=bytes)throw std::runtime_error("Food semantic context is noncanonical");
 return c;
}
inline std::string Unhex(const char* hex) {
 if(!hex)throw std::runtime_error("Food semantic context absent");
 const std::string in(hex);if(in.size()%2)throw std::runtime_error("Food semantic context hex differs");
 const auto digit=[](char c)->unsigned {if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("Food semantic context hex is noncanonical");};
 std::string out;out.reserve(in.size()/2);
 for(size_t n=0;n<in.size();n+=2)out.push_back(static_cast<char>((digit(in[n])<<4)|digit(in[n+1])));
 return out;
}
}
