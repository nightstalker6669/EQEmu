#pragma once
#include "database.h"
#include "repositories/inventory_repository.h"
#include "shared_core/inventory_row_db.h"
#include "shared_core/inventory_session_service.h"
#include <stdexcept>
#include <string>
#include <map>
#include <vector>
#include <set>
// Feature-private physical evidence only. No task/quest/award authority.
namespace KithicorPoisonCustody {
inline auto Q(Database& db,const std::string& sql) {
 auto r=db.QueryDatabase(sql,false);
 if(!r.Success())throw std::runtime_error("Kithicor custody SQL unavailable");
 return r;
}
inline uint64_t N(const char* p) {
 if(!p)throw std::runtime_error("Kithicor custody missing field");
 const std::string s=p;size_t at=0;
 if(s.empty()||s[0]=='-')throw std::runtime_error("Kithicor custody invalid number");
 const auto v=std::stoull(s,&at);if(at!=s.size())throw std::runtime_error("Kithicor custody invalid number");return v;
}
inline std::string Hex(const void* p,size_t n) {
 const auto* bytes=static_cast<const unsigned char*>(p);const char* digits="0123456789ABCDEF";
 std::string out;out.reserve(n*2);for(size_t i=0;i<n;++i){out+=digits[bytes[i]>>4];out+=digits[bytes[i]&15];}return out;
}
inline std::string Hex(const std::string& s){return Hex(s.data(),s.size());}
inline void Word(std::string& out,uint64_t v,unsigned width=4){for(unsigned n=0;n<width;++n)out+=char(v>>(n*8));}
inline void Field(std::string& out,const char* p) {
 if(!p){Word(out,UINT32_MAX);return;}const std::string s=p;Word(out,s.size());out+=s;
}
inline void EvolvingRows(Database& db,uint32_t character,std::string& out) {
 auto installed=Q(db,"SELECT table_name,engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('character_evolving_items','housing_evolving_item_locations') ORDER BY table_name");
 Word(out,installed.RowCount());
 for(auto row=installed.begin();row!=installed.end();++row){
  if(!row[0]||!row[1]||std::string(row[1])!="InnoDB")throw std::runtime_error("Kithicor evolving custody engine differs");
  const std::string table=row[0];Field(out,row[0]);
  const auto columns=table=="character_evolving_items"?
   "id,character_id,item_id,activated,equipped,current_amount,progression,final_item_id,deleted_at":
   "evolving_id,location_kind,character_id,item_id,slot_id,inventory_guid,component,custody_id,path,revision";
  auto values=Q(db,"SELECT "+std::string(columns)+" FROM "+table+" WHERE character_id="+std::to_string(character)+" ORDER BY "+(table=="character_evolving_items"?"id":"evolving_id")+" FOR UPDATE");
  Word(out,values.RowCount());Word(out,values.ColumnCount());
  for(auto value=values.begin();value!=values.end();++value)for(unsigned i=0;i<values.ColumnCount();++i)Field(out,value[i]);
 }
}
inline constexpr const char* CurrencyColumns="platinum,gold,silver,copper,platinum_bank,gold_bank,silver_bank,copper_bank,platinum_cursor,gold_cursor,silver_cursor,copper_cursor,radiant_crystals,career_radiant_crystals,ebon_crystals,career_ebon_crystals";
// Exact SQL NULL/HEX bytes, all inventory rows (including children/queue), and
// currency. Call under the actual session lock; FOR UPDATE locks their preimage.
inline std::string Snapshot(Database& db,uint32_t character) {
 auto rows=InventoryRepository::GetLoadSnapshot(db,character,true);
 if(!rows.success)throw std::runtime_error("Kithicor inventory snapshot unavailable");
 std::string out="KPC2";Word(out,rows.rows.size());
 for(const auto& r:rows.rows)for(const auto& value:r.fields)Field(out,value?value->c_str():nullptr);
 auto coin=Q(db,"SELECT "+std::string(CurrencyColumns)+" FROM character_currency WHERE id="+std::to_string(character)+" FOR UPDATE");
 if(coin.RowCount()!=1||coin.ColumnCount()!=16)throw std::runtime_error("Kithicor currency snapshot unavailable");
 for(unsigned i=0;i<16;++i)Field(out,coin.begin()[i]);
 EvolvingRows(db,character,out);
 if(out.size()>16*1024*1024-1)throw std::runtime_error("Kithicor custody snapshot exceeds journal bound");
 return out;
}
// Optional absence preserves all other builds/domains. Partial/wrong schema is
// unavailable, never Clear. No failing probe query poisons a strict transaction.
inline bool Installed(Database& db) {
 auto r=Q(db,"SELECT COUNT(*),COALESCE(SUM(engine='InnoDB'),0) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='kithicor_poison_custody'");
 if(r.RowCount()!=1)throw std::runtime_error("Kithicor journal schema unavailable");
 if(!N(r.begin()[0]))return false;
 if(N(r.begin()[0])!=1||N(r.begin()[1])!=1)throw std::runtime_error("Kithicor journal engine differs");
 using Shape=std::array<std::string,4>;
 const std::map<std::string,Shape> expected={
  {"request_key",{"char(32)","NO","ascii","ascii_bin"}},
  {"character_id",{"int unsigned","NO","",""}},{"account_id",{"int unsigned","NO","",""}},
  {"inventory_epoch",{"bigint unsigned","NO","",""}},{"inventory_token",{"binary(32)","NO","",""}},
  {"process_key",{"varbinary(96)","NO","",""}},{"client_key",{"varbinary(96)","NO","",""}},
  {"instance_id",{"int unsigned","NO","",""}},{"shared_task_id",{"bigint unsigned","NO","",""}},
  {"phase",{"tinyint unsigned","NO","",""}},{"chest",{"tinyint unsigned","NO","",""}},
  {"npc_type_id",{"int unsigned","NO","",""}},{"npc_entity_id",{"int unsigned","NO","",""}},{"npc_spawn_id",{"int unsigned","NO","",""}},
  {"source_slot",{"int unsigned","NO","",""}},{"source_guid",{"bigint unsigned","NO","",""}},{"source_charges",{"smallint unsigned","NO","",""}},
  {"preimage",{"mediumblob","NO","",""}},{"return_plan",{"mediumblob","NO","",""}},{"postimage",{"mediumblob","YES","",""}},
  {"physical",{"tinyint unsigned","NO","",""}},{"settled",{"tinyint unsigned","NO","",""}},
  {"created_at",{"int unsigned","NO","",""}},{"settled_at",{"int unsigned","YES","",""}}
 };
 auto columns=Q(db,"SELECT column_name,column_type,is_nullable,COALESCE(character_set_name,''),COALESCE(collation_name,''),extra,COALESCE(generation_expression,'') FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='kithicor_poison_custody'");
 if(columns.RowCount()!=expected.size())throw std::runtime_error("Kithicor journal columns differ");
 for(auto row=columns.begin();row!=columns.end();++row){
  for(unsigned i=0;i<5;++i)if(!row[i])throw std::runtime_error("Kithicor journal metadata missing");
  if(!row[5]||*row[5]||!row[6]||*row[6])throw std::runtime_error("Kithicor journal generated/extra column differs");
  auto found=expected.find(row[0]);if(found==expected.end())throw std::runtime_error("Kithicor journal unexpected column");
  std::string type=row[1];
  if(type.rfind("int",0)==0||type.rfind("bigint",0)==0||type.rfind("tinyint",0)==0||type.rfind("smallint",0)==0){const auto open=type.find('('),close=type.find(')');if(open!=std::string::npos&&close>open)type.erase(open,close-open+1);}
  if(Shape{{type,row[2],row[3],row[4]}}!=found->second)throw std::runtime_error("Kithicor journal column shape differs");
 }
 const std::map<std::string,std::vector<std::string>> keys={
  {"PRIMARY",{"0:request_key"}},
  {"actor_pending",{"1:character_id","1:settled"}},
  {"old_session",{"1:character_id","1:inventory_epoch"}},
  {"native_action",{"1:shared_task_id","1:phase","1:chest"}}
 };
 std::map<std::string,std::vector<std::string>> actual;
 auto indices=Q(db,"SELECT index_name,seq_in_index,column_name,non_unique,sub_part,index_type FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='kithicor_poison_custody' ORDER BY index_name,seq_in_index");
 for(auto row=indices.begin();row!=indices.end();++row){if(!row[0]||!row[2]||row[4]||!row[5]||std::string(row[5])!="BTREE")throw std::runtime_error("Kithicor journal key missing/prefixed");auto& key=actual[row[0]];if(N(row[1])!=key.size()+1)throw std::runtime_error("Kithicor journal key order differs");key.push_back(std::to_string(N(row[3]))+":"+row[2]);}
 if(actual!=keys)throw std::runtime_error("Kithicor journal keys differ");
 return true;
}
inline std::string LeaseWhere(const EQEmu::SharedCore::InventorySessions::Lease& lease) {
 return "character_id="+std::to_string(lease.actor.character_id)+" AND account_id="+std::to_string(lease.actor.account_id)+
  " AND inventory_epoch="+std::to_string(lease.epoch)+" AND inventory_token=x'"+Hex(lease.token.data(),lease.token.size())+
  "' AND process_key=x'"+Hex(lease.owner.process_key)+"' AND client_key=x'"+Hex(lease.owner.client_key)+
  "' AND instance_id="+std::to_string(lease.owner.instance_id);
}
inline void LockLease(Database& db,const EQEmu::SharedCore::InventorySessions::Lease& lease) {
 if(!lease.active||lease.owner.zone_id!=457||!lease.owner.instance_id)throw std::runtime_error("Kithicor actual lease absent");
 auto r=Q(db,"SELECT account_id,epoch,HEX(token),HEX(process_key),HEX(client_key),zone_id,instance_id,active FROM housing_inventory_sessions WHERE character_id="+std::to_string(lease.actor.character_id)+" FOR UPDATE");
 if(r.RowCount()!=1)throw std::runtime_error("Kithicor actual session absent");auto h=r.begin();
 if(N(h[0])!=lease.actor.account_id||N(h[1])!=lease.epoch||!h[2]||std::string(h[2])!=Hex(lease.token.data(),lease.token.size())||!h[3]||std::string(h[3])!=Hex(lease.owner.process_key)||!h[4]||std::string(h[4])!=Hex(lease.owner.client_key)||N(h[5])!=457||N(h[6])!=lease.owner.instance_id||N(h[7])!=1)throw std::runtime_error("Kithicor actual session changed");
}
inline bool Pending(Database& db,uint32_t character) {
 if(!Installed(db))return false;
 auto r=Q(db,"SELECT request_key FROM kithicor_poison_custody WHERE character_id="+std::to_string(character)+" AND settled=0 FOR UPDATE");
 return r.RowCount()!=0;
}
using RawRow=EQEmu::SharedCore::InventoryRow;
inline RawRow Raw(const InventoryRepository::LoadRow& source) {
 RawRow row;row.values=source.values;
 if(!source.fields[2])row.null_fields|=EQEmu::SharedCore::NullItemId;
 if(!source.fields[3])row.null_fields|=EQEmu::SharedCore::NullCharges;
 if(!source.fields[12])row.null_fields|=EQEmu::SharedCore::NullCustomData;
 if(!source.fields[16])row.null_fields|=EQEmu::SharedCore::NullGuid;
 return row;
}
struct ReturnPlan {
 std::vector<std::pair<uint32_t,RawRow>> rows; // source -> exact durable destination
 std::array<uint32_t,4> refund{}; // platinum,gold,silver,copper
};
inline std::string Encode(const ReturnPlan& plan) {
 using namespace EQEmu::SharedCore;
 InventoryCodec::Bytes out;InventoryCodec::W(out,1,4);InventoryCodec::W(out,plan.rows.size(),4);
 for(const auto& entry:plan.rows){InventoryCodec::W(out,entry.first,4);auto row=InventoryCodec::Encode(entry.second);InventoryCodec::W(out,row.size(),4);out.insert(out.end(),row.begin(),row.end());}
 for(auto amount:plan.refund)InventoryCodec::W(out,amount,4);
 if(out.size()>InventoryCodec::MaximumPayload)throw std::runtime_error("Kithicor return plan exceeds journal bound");
 return std::string(out.begin(),out.end());
}
inline ReturnPlan DecodePlan(const char* hex) {
 if(!hex)throw std::runtime_error("Kithicor return plan absent");const std::string text=hex;
 if(text.size()%2||text.size()>2*(16*1024*1024-1))throw std::runtime_error("Kithicor return plan bound");
 const auto digit=[](char c)->uint8_t {if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;throw std::runtime_error("Kithicor return plan encoding");};
 EQEmu::SharedCore::InventoryCodec::Bytes bytes;bytes.reserve(text.size()/2);
 for(size_t i=0;i<text.size();i+=2)bytes.push_back(uint8_t((digit(text[i])<<4)|digit(text[i+1])));
 EQEmu::SharedCore::InventoryCodec::Reader reader{bytes};ReturnPlan out;
 if(reader.N(4)!=1)throw std::runtime_error("Kithicor return plan version");const auto count=reader.N(4);
 if(count>uint32_t(EQ::invbag::CURSOR_BAG_COUNT))throw std::runtime_error("Kithicor return plan capacity");
 for(uint32_t i=0;i<count;++i){const uint32_t source=reader.N(4);auto row=EQEmu::SharedCore::InventoryCodec::Decode(reader.Blob(EQEmu::SharedCore::InventoryCodec::MaximumPayload));out.rows.emplace_back(source,std::move(row));}
 for(auto& amount:out.refund)amount=reader.N(4);
 if(reader.at!=bytes.size()||Encode(out)!=std::string(bytes.begin(),bytes.end()))throw std::runtime_error("Kithicor noncanonical return plan");
 return out;
}
inline bool ReturnSlot(uint32_t slot) {
 return slot==EQ::invslot::slotCursor||
  (slot>=EQ::invbag::CURSOR_BAG_BEGIN&&slot<=EQ::invbag::CURSOR_BAG_END)||
  (slot>=EQ::invslot::TRADE_BEGIN&&slot<=EQ::invslot::TRADE_NPC_END)||
  (slot>=EQ::invbag::TRADE_BAGS_BEGIN&&slot<EQ::invbag::TRADE_BAGS_BEGIN+4*EQ::invbag::SLOT_COUNT);
}
// Exact old writer is gone. This is a bounded physical return, using the
// existing canonical inventory row writer/codec, never a quest item issuance.
// The caller has already proved the whole preimage and holds the actual lease.
inline void AbortReturns(Database& db,uint32_t character,const ReturnPlan& plan) {
 auto engines=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('inventory','character_currency','kithicor_poison_custody','kithicor_mission_actions','housing_inventory_sessions')");
 if(engines.RowCount()!=1||N(engines.begin()[0])!=5)throw std::runtime_error("Kithicor abort foundation differs");
 auto before=InventoryRepository::GetLoadSnapshot(db,character,true);if(!before.success)throw std::runtime_error("Kithicor abort inventory unavailable");
 std::map<uint32_t,RawRow> original,expected;
 for(const auto& entry:before.rows){auto row=Raw(entry);original.emplace(row.values.slot_id,row);if(!ReturnSlot(row.values.slot_id))expected.emplace(row.values.slot_id,std::move(row));}
 std::set<uint32_t> sources;size_t index=0;
 for(const auto& entry:plan.rows){
  const auto slot=index?EQ::invbag::CURSOR_BAG_BEGIN+index:EQ::invslot::slotCursor;++index;
  auto source=original.find(entry.first);
  if(!ReturnSlot(entry.first)||!sources.insert(entry.first).second||source==original.end()||entry.second.values.character_id!=character||entry.second.values.slot_id!=slot||!entry.second.children.empty()||!entry.second.evolving_payload.empty())throw std::runtime_error("Kithicor abort destination not bound");
  auto moved=source->second;moved.values.slot_id=slot;
  if(EQEmu::SharedCore::InventoryCodec::Encode(moved)!=EQEmu::SharedCore::InventoryCodec::Encode(entry.second)||!expected.emplace(slot,moved).second)throw std::runtime_error("Kithicor abort original row differs");
 }
 for(const auto& entry:original)if(ReturnSlot(entry.first)&&!sources.count(entry.first))throw std::runtime_error("Kithicor abort omitted original property");
 auto coins=Q(db,"SELECT "+std::string(CurrencyColumns)+" FROM character_currency WHERE id="+std::to_string(character)+" FOR UPDATE");
 if(coins.RowCount()!=1)throw std::runtime_error("Kithicor abort currency absent");std::array<uint64_t,16> after{};
 for(unsigned i=0;i<16;++i){after[i]=N(coins.begin()[i]);if(i<4){after[i]+=plan.refund[i];if(after[i]>INT32_MAX)throw std::runtime_error("Kithicor abort currency overflow");}}
 Q(db,fmt::format("DELETE FROM inventory WHERE character_id={} AND(slot_id={} OR slot_id BETWEEN {} AND {} OR slot_id BETWEEN {} AND {} OR slot_id BETWEEN {} AND {})",character,EQ::invslot::slotCursor,EQ::invbag::CURSOR_BAG_BEGIN,EQ::invbag::CURSOR_BAG_END,EQ::invslot::TRADE_BEGIN,EQ::invslot::TRADE_NPC_END,EQ::invbag::TRADE_BAGS_BEGIN,EQ::invbag::TRADE_BAGS_BEGIN+4*EQ::invbag::SLOT_COUNT-1));
 auto locator=Q(db,"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_evolving_item_locations'");
 for(const auto& entry:plan.rows){
  Q(db,"INSERT INTO inventory("+std::string(EQEmu::SharedCore::InventoryRowDB::WriteColumns)+") VALUES("+EQEmu::SharedCore::InventoryRowDB::Values(entry.second,entry.second.values.slot_id)+")");
  if(N(locator.begin()[0])&&entry.first!=entry.second.values.slot_id)Q(db,fmt::format("UPDATE housing_evolving_item_locations SET slot_id={},revision=revision+1 WHERE character_id={} AND location_kind=1 AND slot_id={} AND inventory_guid={}",entry.second.values.slot_id,character,entry.first,entry.second.values.guid));
 }
 Q(db,fmt::format("UPDATE character_currency SET platinum={},gold={},silver={},copper={} WHERE id={}",after[0],after[1],after[2],after[3],character));
 auto actual=InventoryRepository::GetLoadSnapshot(db,character,true);if(!actual.success||actual.rows.size()!=expected.size())throw std::runtime_error("Kithicor abort return count differs");
 for(const auto& entry:actual.rows){auto row=Raw(entry);auto found=expected.find(row.values.slot_id);if(found==expected.end()||EQEmu::SharedCore::InventoryCodec::Encode(found->second)!=EQEmu::SharedCore::InventoryCodec::Encode(row))throw std::runtime_error("Kithicor abort return postimage differs");}
 auto coin_after=Q(db,"SELECT "+std::string(CurrencyColumns)+" FROM character_currency WHERE id="+std::to_string(character)+" FOR UPDATE");if(coin_after.RowCount()!=1)throw std::runtime_error("Kithicor abort currency missing");
 for(unsigned i=0;i<16;++i)if(N(coin_after.begin()[i])!=after[i])throw std::runtime_error("Kithicor abort currency postimage differs");
}
// ONLY after actual complete Client deletion OR inside the existing exact
// supervisor-death transaction after history/current/death proof locks.
// Callers own that proof boundary. No credit/ACK is produced.
inline void ReconcilePhysicalUnderTransaction(Database& db,const EQEmu::SharedCore::InventorySessions::Lease& lease) {
 if(!Installed(db))return;
 auto rows=Q(db,"SELECT request_key,HEX(preimage),HEX(postimage),physical,HEX(return_plan) FROM kithicor_poison_custody WHERE character_id="+std::to_string(lease.actor.character_id)+" AND settled=0 FOR UPDATE");
 if(!rows.RowCount())return;
 if(rows.RowCount()!=1)throw std::runtime_error("Kithicor multiple pending custody outcomes");
 LockLease(db,lease);
 auto row=rows.begin();if(!row[0]||!row[1])throw std::runtime_error("Kithicor custody evidence absent");
 const std::string key=row[0];const auto physical=N(row[3]);
 auto bound=Q(db,"SELECT request_key FROM kithicor_poison_custody WHERE request_key='"+db.Escape(key)+"' AND "+LeaseWhere(lease)+" AND phase=0 AND chest BETWEEN 1 AND 6 FOR UPDATE");
 if(bound.RowCount()!=1||(physical!=0&&physical!=1&&physical!=3))throw std::runtime_error("Kithicor old session evidence differs");
 auto action=Q(db,"SELECT a.status FROM kithicor_mission_actions a JOIN kithicor_poison_custody j ON j.shared_task_id=a.shared_task_id AND j.phase=a.phase AND j.chest=a.action_id AND j.character_id=a.character_id WHERE j.request_key='"+db.Escape(key)+"' AND a.item_id=38504 FOR UPDATE");
 if(physical!=3&&(action.RowCount()!=1||N(action.begin()[0])>1||(!physical&&N(action.begin()[0])!=0)))throw std::runtime_error("Kithicor owned action reservation changed");
 const auto actual=Hex(Snapshot(db,lease.actor.character_id));
 if(physical==0||physical==3) {
  if(row[2]||actual!=row[1])throw std::runtime_error("Kithicor no-debit preimage not proved");
  AbortReturns(db,lease.actor.character_id,DecodePlan(row[4]));
  // This attempt held the unique action at status 0. Exact original lease and
  // unchanged physical preimage prove there was no debit; release only its own
  // reservation, never another character's success or any gameplay credit.
  if(physical==0){auto removed=Q(db,"DELETE a FROM kithicor_mission_actions a JOIN kithicor_poison_custody j ON j.shared_task_id=a.shared_task_id AND j.phase=a.phase AND j.chest=a.action_id AND j.character_id=a.character_id WHERE j.request_key='"+db.Escape(key)+"' AND a.item_id=38504 AND a.status=0");
   if(removed.RowsAffected()!=1)throw std::runtime_error("Kithicor owned no-debit reservation not removed");}
 } else if(!row[2]||actual!=row[2])throw std::runtime_error("Kithicor committed postimage not proved");
 // physical=1 already has all returns. physical=0 has just returned everything
 // without consuming a vial. Both transitions settle atomically with this row;
 // no task callbacks or gameplay publication are replayed on writer death.
 const auto resolved=Snapshot(db,lease.actor.character_id);
 auto done=Q(db,"UPDATE kithicor_poison_custody SET settled=1,physical="+std::to_string(physical==1?1:2)+",postimage=x'"+Hex(resolved)+"',settled_at=UNIX_TIMESTAMP() WHERE request_key='"+db.Escape(key)+"' AND settled=0 AND "+LeaseWhere(lease));
 if(done.RowsAffected()!=1)throw std::runtime_error("Kithicor physical reconciliation changed");
 }
}
