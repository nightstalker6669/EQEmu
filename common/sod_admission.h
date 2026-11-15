#pragma once
#include "common/sod_reward_quote.h"
#include <array>
#include <chrono>
#include <cstring>
#include <set>
#include <vector>
namespace SodAdmission {
inline uint64_t Milliseconds() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
struct Proof {
 uint32_t magic=0x534f4432,version=1,zone=0,instance=0,requester=0,task=0;
 uint64_t nonce=0,created_ms=0;
 uint32_t player_count=0,merc_count=0;
 uint32_t players[6]{},mercs[6]{},owners[6]{};
};
inline bool Shape(const Proof& p) {
 if(p.magic!=0x534f4432||p.version!=1||!p.zone||!p.nonce||!p.created_ms||!p.requester||
  !SodRewardQuote::Shared(p.task)||!p.player_count||p.player_count>6||p.merc_count>6||p.player_count+p.merc_count<1||p.player_count+p.merc_count>6)return false;
 std::set<uint32_t> players,mercs,owners;
 for(unsigned i=0;i<6;++i) {
  if(i<p.player_count){if(!p.players[i]||!players.insert(p.players[i]).second)return false;}else if(p.players[i])return false;
  if(i<p.merc_count){if(!p.mercs[i]||!p.owners[i]||!mercs.insert(p.mercs[i]).second||!owners.insert(p.owners[i]).second)return false;}
  else if(p.mercs[i]||p.owners[i])return false;
 }
 if(!players.count(p.requester))return false;
 for(const auto owner:owners)if(!players.count(owner))return false;
 return true;
}
inline bool Fresh(const Proof& p) { auto now=Milliseconds();return now>=p.created_ms&&now-p.created_ms<=5000; }
inline bool Matches(const Proof& p,const std::vector<uint32_t>& ids) {
 return Shape(p)&&Fresh(p)&&std::set<uint32_t>(ids.begin(),ids.end())==std::set<uint32_t>(p.players,p.players+p.player_count);
}
}
