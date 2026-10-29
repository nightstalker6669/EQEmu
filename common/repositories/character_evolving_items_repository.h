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

#include "common/repositories/base/base_character_evolving_items_repository.h"

#include "common/database.h"
#include "common/strings.h"

class CharacterEvolvingItemsRepository: public BaseCharacterEvolvingItemsRepository {
public:
	// Custom extended repository methods here
	static int UpdateOne(
		Database& db,
		const CharacterEvolvingItems &e
	)
	{
		// Normal core writers must never reactivate, evolve or transfer a held UID.
		// Optional housing installation keeps the upstream behavior when absent.
		auto schema = db.QueryDatabase("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name='housing_evolving_item_locations'");
		if (!schema.Success() || schema.RowCount() != 1) return 0;
		const bool held_guard = std::string(schema.begin()[0]) != "0";
		std::vector<std::string> v;

		auto columns = Columns();
		if (held_guard) for (auto& column : columns) column = TableName() + "." + column;

		v.push_back(columns[1] + " = " + std::to_string(e.character_id));
		v.push_back(columns[2] + " = " + std::to_string(e.item_id));
		v.push_back(columns[3] + " = " + std::to_string(e.activated));
		v.push_back(columns[4] + " = " + std::to_string(e.equipped));
		v.push_back(columns[5] + " = " + std::to_string(e.current_amount));
		v.push_back(columns[6] + " = " + std::to_string(e.progression));
		v.push_back(columns[7] + " = " + std::to_string(e.final_item_id));
		v.push_back(columns[8] + " = FROM_UNIXTIME(" + (e.deleted_at > 0 ? std::to_string(e.deleted_at) : "null") + ")");

		auto results = db.QueryDatabase(
			fmt::format(
				"UPDATE {} SET {} WHERE {} = {}{}",
				held_guard ? TableName() + " LEFT JOIN housing_evolving_item_locations housing_evolving_guard ON housing_evolving_guard.evolving_id=character_evolving_items.id" : TableName(),
				Strings::Implode(", ", v),
				held_guard ? TableName() + "." + PrimaryKey() : PrimaryKey(),
				e.id,
				held_guard ? " AND (housing_evolving_guard.evolving_id IS NULL OR housing_evolving_guard.location_kind<>2)" : ""
			)
		);

		return (results.Success() ? results.RowsAffected() : 0);
	}


	static CharacterEvolvingItems SetCurrentAmountAndProgression(Database& db, const uint64 id, const uint64 amount, const double progression)
	{
		auto e = FindOne(db, id);
		if (e.id == 0) {
			return NewEntity();
		}

		e.current_amount = amount;
		e.progression    = progression;
		e.deleted_at     = 0;
		UpdateOne(db, e);
		return e;
	}

	static CharacterEvolvingItems SetEquipped(Database& db, const uint64 id, const bool equipped)
	{
		auto e = FindOne(db, id);
		if (e.id == 0) {
			return NewEntity();
		}

		e.equipped   = equipped;
		e.deleted_at = 0;
		UpdateOne(db, e);
		return e;
	}

	static CharacterEvolvingItems SoftDelete(Database& db, const uint64 id)
	{
		auto e = FindOne(db, id);
		if (e.id == 0) {
			return NewEntity();
		}

		e.deleted_at = time(nullptr);
		UpdateOne(db, e);
		return e;
	}

	static bool UpdateCharID(Database &db, const uint64 id, const uint32 to_char_id)
	{
		auto e = FindOne(db, id);
		if (e.id == 0) {
			return false;
		}

		e.character_id = to_char_id;
		e.deleted_at   = 0;
		return UpdateOne(db, e);
	}
};
