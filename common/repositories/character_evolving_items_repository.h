#ifndef EQEMU_CHARACTER_EVOLVING_ITEMS_REPOSITORY_H
#define EQEMU_CHARACTER_EVOLVING_ITEMS_REPOSITORY_H

#include "../database.h"
#include "../strings.h"
#include "base/base_character_evolving_items_repository.h"

#include <boost/tuple/tuple_comparison.hpp>

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

#endif //EQEMU_CHARACTER_EVOLVING_ITEMS_REPOSITORY_H
