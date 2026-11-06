/*	EQEmu: EQEmulator

	Copyright (C) 2001-2026 EQEmu Development Team

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "common/repositories/base/base_inventory_repository.h"

#include "common/database.h"
#include "common/strings.h"

#include <array>
#include <algorithm>
#include <charconv>
#include <optional>
#include <set>

class InventoryRepository: public BaseInventoryRepository {
public:

    /**
     * This file was auto generated and can be modified and extended upon
     *
     * Base repository methods are automatically
     * generated in the "base" version of this repository. The base repository
     * is immutable and to be left untouched, while methods in this class
     * are used as extension methods for more specific persistence-layer
     * accessors or mutators.
     *
     * Base Methods (Subject to be expanded upon in time)
     *
     * Note: Not all tables are designed appropriately to fit functionality with all base methods
     *
     * InsertOne
     * UpdateOne
     * DeleteOne
     * FindOne
     * GetWhere(std::string where_filter)
     * DeleteWhere(std::string where_filter)
     * InsertMany
     * All
     *
     * Example custom methods in a repository
     *
     * InventoryRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * InventoryRepository::GetWhereNeverExpires()
     * InventoryRepository::GetWhereXAndY()
     * InventoryRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	// Custom extended repository methods here
	static bool TryDeleteWhere(Database &db, const std::string &where_filter)
	{
		const auto result = db.QueryDatabase(fmt::format(
			"DELETE FROM {} WHERE {}",
			TableName(),
			where_filter
		));

		// Generated DeleteWhere returns zero for both a successful no-op and an
		// SQL failure. Persistence callers need to distinguish those outcomes.
		return result.Success();
	}

 // Exact load snapshot. custom_data is selected as HEX so embedded NULs are
 // retained and SQL NULL stays distinct from an empty value. No generated base
 // repository code is changed.
 using LoadFields = std::array<std::optional<std::string>,17>;
 struct LoadRow { Inventory values{}; LoadFields fields{}; };
 struct LoadSnapshot { bool success=false; std::vector<LoadRow> rows; };
 enum class GuidLoadResult { Applied, Conflict, DatabaseFailure, CommitUncertain };

 static bool DecodeLoadRow(const LoadFields& fields,LoadRow& output) {
  LoadRow row;row.fields=fields;
  auto unsigned_value=[&](size_t i,uint64_t maximum,uint64_t& value) {
   if(!fields[i]){value=0;return i==2||i==3||i==16;}
   const auto& s=*fields[i];value=0;
   auto parsed=std::from_chars(s.data(),s.data()+s.size(),value);
   return !s.empty()&&parsed.ec==std::errc{}&&parsed.ptr==s.data()+s.size()&&value<=maximum;
  };
  uint64_t values[17]{};
  for(size_t i=0;i<17;++i){
   if(i==12||i==15)continue;
   const auto maximum=i==16?UINT64_MAX:(i==3?UINT16_MAX:(i==11?UINT8_MAX:UINT32_MAX));
   if(!unsigned_value(i,maximum,values[i]))return false;
  }
  if(!fields[15])return false;
  const auto& hero=*fields[15];int32_t signed_hero=0;
  auto parsed=std::from_chars(hero.data(),hero.data()+hero.size(),signed_hero);
  if(hero.empty()||parsed.ec!=std::errc{}||parsed.ptr!=hero.data()+hero.size())return false;
  if(fields[12]){
   const auto& hex=*fields[12];if(hex.size()%2||hex.size()>131070)return false;
   auto digit=[](char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='A'&&c<='F')return c-'A'+10;if(c>='a'&&c<='f')return c-'a'+10;return -1;};
   for(size_t i=0;i<hex.size();i+=2){auto a=digit(hex[i]),b=digit(hex[i+1]);if(a<0||b<0)return false;row.values.custom_data.push_back(char((a<<4)|b));}
  }
  auto& v=row.values;
  v.character_id=values[0];v.slot_id=values[1];v.item_id=values[2];v.charges=values[3];v.color=values[4];
  v.augment_one=values[5];v.augment_two=values[6];v.augment_three=values[7];v.augment_four=values[8];v.augment_five=values[9];v.augment_six=values[10];
  v.instnodrop=values[11];v.ornament_icon=values[13];v.ornament_idfile=values[14];v.ornament_hero_model=signed_hero;v.guid=values[16];
  output=std::move(row);return true;
 }

 static LoadSnapshot GetLoadSnapshot(Database& db,uint32_t character,bool lock=false) {
  LoadSnapshot out;if(!character)return out;
  auto columns=SelectColumns();columns[12]="HEX(custom_data)";
  auto result=db.QueryDatabase("SELECT "+Strings::Implode(",",columns)+" FROM inventory WHERE character_id="+std::to_string(character)+" ORDER BY slot_id"+(lock?" FOR UPDATE":""),false);
  if(!result.Success()||result.ColumnCount()!=17)return out;
  std::set<uint32_t> slots;
  for(auto r=result.begin();r!=result.end();++r){
   LoadFields fields;for(size_t i=0;i<17;++i)if(r[i])fields[i]=std::string(r[i]);
   LoadRow row;if(!DecodeLoadRow(fields,row)||row.values.character_id!=character||!slots.insert(row.values.slot_id).second)return {};
   out.rows.push_back(std::move(row));
  }
  out.success=true;return out;
 }

 // Short, owned transaction AFTER memory loading. Lock and compare every raw
 // row before any GUID-only update. Never REPLACE rows or normalize NULLs.
 // Caller must reject partial admission on failure, including uncertain COMMIT;
 // this operation never retries a write and is not an item custody transition.
 static GuidLoadResult UpdateLoadGuids(Database& db,uint32_t character,const LoadSnapshot& expected,
  const std::vector<Inventory>& loaded) {
  if(!character||!expected.success)return GuidLoadResult::Conflict;
  std::set<uint32_t> slots;std::set<uint64_t> serials;
  for(const auto& v:loaded)if(v.character_id!=character||!v.guid||v.guid>INT32_MAX||!slots.insert(v.slot_id).second||!serials.insert(v.guid).second)return GuidLoadResult::Conflict;
  auto engine=db.QueryDatabase(std::string("SELECT engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='inventory'"),false);
  if(!engine.Success()||engine.RowCount()!=1||!engine.begin()[0]||std::string(engine.begin()[0])!="InnoDB")return GuidLoadResult::DatabaseFailure;
  if(!db.QueryDatabase(std::string("START TRANSACTION"),false).Success())return GuidLoadResult::DatabaseFailure;
  auto rollback=[&](GuidLoadResult error){db.QueryDatabase(std::string("ROLLBACK"),false);return error;};
  bool committing=false;
  try {
   auto current=GetLoadSnapshot(db,character,true);
   if(!current.success)return rollback(GuidLoadResult::DatabaseFailure);
   if(current.rows.size()!=expected.rows.size())return rollback(GuidLoadResult::Conflict);
   for(size_t i=0;i<current.rows.size();++i)if(current.rows[i].fields!=expected.rows[i].fields)return rollback(GuidLoadResult::Conflict);
   for(const auto& v:loaded){
    const auto found=std::find_if(current.rows.begin(),current.rows.end(),[&](const LoadRow& r){return r.values.slot_id==v.slot_id;});
    if(found==current.rows.end()||found->values.item_id!=v.item_id)return rollback(GuidLoadResult::Conflict);
    if(found->fields[16]&&found->values.guid==v.guid)continue;
    auto changed=db.QueryDatabase("UPDATE inventory SET guid="+std::to_string(v.guid)+" WHERE character_id="+std::to_string(character)+" AND slot_id="+std::to_string(v.slot_id),false);
    if(!changed.Success()||changed.RowsAffected()!=1)return rollback(GuidLoadResult::DatabaseFailure);
   }
   committing=true;
   if(!db.QueryDatabase(std::string("COMMIT"),false).Success())return GuidLoadResult::CommitUncertain;
   return GuidLoadResult::Applied;
  }catch(...){return committing?GuidLoadResult::CommitUncertain:rollback(GuidLoadResult::DatabaseFailure);}
 }

};
