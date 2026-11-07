#pragma once
#include <cstdint>
#include <memory>
class Client;class Object;struct DBTradeskillRecipe_Struct;
namespace EQ {class ItemInstance;}
namespace RechargeRecipe8 {
struct Cost {uint32_t item,units;};
struct Definition {uint32_t recipe,input,output,input_cap,charges,trivial;Cost costs[2];};
inline constexpr Definition Definitions[]={
 {3102883,64681,64681,5,5,0,{{9181,1},{0,0}}},
 {3102884,97921,97921,60,60,15,{{14510,1},{14802,1}}},
 {3102885,29502,75857,5,10,0,{{58244,2},{0,0}}},
 {3102886,75857,75857,10,10,0,{{58244,2},{0,0}}},
 {3102887,97922,97922,5,5,0,{{14802,1},{16905,1}}},
 {3102888,96494,96494,20,20,0,{{58244,1},{0,0}}},
 {3102889,75857,29502,10,5,0,{{14804,1},{0,0}}},
 {3102890,81303,81303,5,5,0,{{34467,1},{0,0}}},
};
inline const Definition* Find(uint32_t recipe)noexcept {
 for(const auto& d:Definitions)if(d.recipe==recipe)return &d;
 return nullptr;
}
class Custody;
class Proof final {
 friend class Custody;
 Client* client_;uint32_t recipe_,character_,account_;bool claimed_=false;
 Proof(Client&,uint32_t);
public:
 ~Proof()=default;Proof(const Proof&)=delete;Proof&operator=(const Proof&)=delete;
};
class Custody final {
 static thread_local Proof* current_;
public:
 // Only actual owned root Toolbox17902; no World/cursor/automatic writer.
 static std::unique_ptr<Proof> Manual(Client&,DBTradeskillRecipe_Struct&,Object*,EQ::ItemInstance*,int16_t)noexcept;
 static bool Claim(Client&,const DBTradeskillRecipe_Struct&)noexcept;
 static bool Complete(Client&,std::unique_ptr<Proof>,DBTradeskillRecipe_Struct&);
};
}
