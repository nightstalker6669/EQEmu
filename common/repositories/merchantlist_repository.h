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

#include "common/repositories/base/base_merchantlist_repository.h"

#include "common/database.h"
#include "common/strings.h"

#include <utility>

class MerchantlistRepository: public BaseMerchantlistRepository {
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
     * MerchantlistRepository::GetByZoneAndVersion(int zone_id, int zone_version)
     * MerchantlistRepository::GetWhereNeverExpires()
     * MerchantlistRepository::GetWhereXAndY()
     * MerchantlistRepository::DeleteWhereXAndY()
     *
     * Most of the above could be covered by base methods, but if you as a developer
     * find yourself re-using logic for other parts of the code, its best to just make a
     * method that can be re-used easily elsewhere especially if it can use a base repository
     * method and encapsulate filters there
     */

	struct MerchantlistWithLanguage : Merchantlist {
		int16_t language_required = -1;
		uint16_t language_skill_required = 0;
	};

	// One catalogue query keeps the language policy and item row in the same
	// snapshot. The generated base repository remains unchanged.
	static std::vector<MerchantlistWithLanguage> GetWhereWithLanguage(
		Database &db, const std::string &where_filter
	)
	{
		std::vector<MerchantlistWithLanguage> entries;
		auto results = db.QueryDatabase(fmt::format(
			"SELECT {}, language_required, language_skill_required FROM {} WHERE {}",
			SelectColumnsRaw(), TableName(), where_filter
		));
		if (!results.Success()) {
			return entries;
		}
		entries.reserve(results.RowCount());
		for (auto row = results.begin(); row != results.end(); ++row) {
			MerchantlistWithLanguage e{};
			e.merchantid             = row[0] ? static_cast<int32_t>(atoi(row[0])) : 0;
			e.slot                   = row[1] ? static_cast<uint32_t>(strtoul(row[1], nullptr, 10)) : 0;
			e.item                   = row[2] ? static_cast<int32_t>(atoi(row[2])) : 0;
			e.faction_required       = row[3] ? static_cast<int16_t>(atoi(row[3])) : -100;
			e.level_required         = row[4] ? static_cast<uint8_t>(strtoul(row[4], nullptr, 10)) : 0;
			e.min_status             = row[5] ? static_cast<uint8_t>(strtoul(row[5], nullptr, 10)) : 0;
			e.max_status             = row[6] ? static_cast<uint8_t>(strtoul(row[6], nullptr, 10)) : 255;
			e.alt_currency_cost      = row[7] ? static_cast<uint16_t>(strtoul(row[7], nullptr, 10)) : 0;
			e.classes_required       = row[8] ? static_cast<int32_t>(atoi(row[8])) : 65535;
			e.probability            = row[9] ? static_cast<int32_t>(atoi(row[9])) : 100;
			e.bucket_name            = row[10] ? row[10] : "";
			e.bucket_value           = row[11] ? row[11] : "";
			e.bucket_comparison      = row[12] ? static_cast<uint8_t>(strtoul(row[12], nullptr, 10)) : 0;
			e.min_expansion          = row[13] ? static_cast<int8_t>(atoi(row[13])) : -1;
			e.max_expansion          = row[14] ? static_cast<int8_t>(atoi(row[14])) : -1;
			e.content_flags          = row[15] ? row[15] : "";
			e.content_flags_disabled = row[16] ? row[16] : "";
			e.language_required = row[17] ? static_cast<int16_t>(atoi(row[17])) : -2;
			e.language_skill_required = row[18] ? static_cast<uint16_t>(strtoul(row[18], nullptr, 10)) : 101;
			entries.push_back(std::move(e));
		}
		return entries;
	}

};
