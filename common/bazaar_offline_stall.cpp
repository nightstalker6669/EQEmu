#include "bazaar_offline_stall.h"

#include "database.h"
#include "eq_constants.h"
#include "repositories/trader_repository.h"
#include "strings.h"

#include <cmath>
#include <cstdlib>
#include <mysqld_error.h>

namespace OfflineBazaarStall {
namespace {
constexpr uint8_t Attached = 1;
constexpr uint8_t ParkedPhase = 2;
constexpr uint8_t Restored = 3;
constexpr uint8_t ParkedListing = TraderRepository::OfflineParked;

struct StorageError {};

MySQLRequestResult Query(Database &db, const std::string &sql)
{
	auto result = db.QueryDatabase(sql, false);
	if (!result.Success()) {
		throw StorageError{};
	}
	return result;
}

class Transaction {
public:
	explicit Transaction(Database &db) : db_(db)
	{
		if (!db_.TransactionBeginStrict().Success()) {
			throw StorageError{};
		}
	}

	~Transaction()
	{
		if (open_) {
			db_.TransactionRollbackStrict();
		}
	}

	bool Commit()
	{
		open_ = false;
		return db_.TransactionCommitStrict().Success();
	}

private:
	Database &db_;
	bool open_ = true;
};

bool ValidOwner(const std::string &owner)
{
	return owner.size() == 32 && owner.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
}

bool ValidPosition(const Position &p)
{
	return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z) && std::isfinite(p.heading);
}
}

Storage CheckStorage(Database &db)
{
	auto exists = db.QueryDatabase(std::string("SELECT phase FROM bazaar_offline_stalls LIMIT 0"), false);
	if (!exists.Success()) {
		return exists.ErrorNumber() == ER_NO_SUCH_TABLE ? Storage::Legacy : Storage::Unavailable;
	}
	auto columns = db.QueryDatabase(std::string(
		"SELECT COUNT(*) FROM information_schema.columns c "
		"JOIN information_schema.tables t USING (table_schema,table_name) "
		"WHERE c.table_schema=DATABASE() AND t.engine='InnoDB' AND "
		"((c.table_name='bazaar_offline_stalls' AND c.column_name IN ('owner','instance_version','instance_epoch')) OR "
		"(c.table_name='trader' AND c.column_name IN ('offline_inventory_slot','offline_satchel_slot','offline_satchel_guid')) OR "
		"(c.table_name='bazaar_offline_parcel_quotes' AND c.column_name IN ('delivery_slot','delivery_serial')) OR "
		"(c.table_name='instance_list' AND c.column_name='offline_bazaar_epoch'))"
	), false);
	return columns.Success() && columns.RowCount() == 1 && Strings::ToUnsignedInt(columns.begin()[0]) == 9
		? Storage::Ready : Storage::Unavailable;
}

std::optional<std::string> Register(Database &db, const Stall &stall, uint32_t entity,
	const std::map<uint32_t, ItemLocation> &slots)
{
	if (!entity || !stall.account || !stall.seller || slots.empty() || !ValidPosition(stall.position) ||
		CheckStorage(db) != Storage::Ready) {
		return {};
	}

	try {
		Transaction transaction(db);
		auto token = Query(db, "SELECT REPLACE(UUID(),'-','')");
		if (token.RowCount() != 1 || !token.begin()[0]) {
			return {};
		}
		const std::string owner = token.begin()[0];
		if (!ValidOwner(owner)) {
			return {};
		}

		std::string epoch_sql = "NULL";
		if (stall.instance) {
			const auto permanent = fmt::format(
				"id={} AND zone={} AND version={} AND is_global=1 AND never_expires=1",
				stall.instance, Zones::BAZAAR, stall.version
			);
			Query(db, fmt::format(
				"UPDATE instance_list SET offline_bazaar_epoch=UNHEX(REPLACE(UUID(),'-','')) "
				"WHERE {} AND offline_bazaar_epoch IS NULL", permanent
			));
			auto value = Query(db, fmt::format(
				"SELECT HEX(offline_bazaar_epoch) FROM instance_list WHERE {} FOR UPDATE", permanent
			));
			if (value.RowCount() != 1 || !value.begin()[0] || !ValidOwner(value.begin()[0])) {
				return {};
			}
			epoch_sql = fmt::format("UNHEX('{}')", value.begin()[0]);
		}
		auto inserted = Query(db, fmt::format(
			"INSERT INTO bazaar_offline_stalls (character_id,account_id,instance_id,instance_version,instance_epoch,owner,phase,x,y,z,heading) "
			"SELECT id,account_id,{},{},{},UNHEX('{}'),{},{},{},{},{} FROM character_data WHERE id={} AND account_id={}",
			stall.instance, stall.version, epoch_sql, owner, Attached, stall.position.x, stall.position.y, stall.position.z,
			stall.position.heading, stall.seller, stall.account
		));
		if (inserted.RowsAffected() != 1) {
			return {};
		}
		auto listings = Query(db, fmt::format(
			"SELECT id,item_sn FROM trader WHERE char_id={} FOR UPDATE", stall.seller
		));
		if (listings.RowCount() != slots.size()) {
			return {};
		}
		for (auto listing : listings) {
			const auto serial = Strings::ToUnsignedInt(listing[1]);
			const auto slot = slots.find(serial);
			if (slot == slots.end() || !slot->second.slot || !slot->second.satchel_slot || !slot->second.satchel_guid) {
				return {};
			}
			auto updated = Query(db, fmt::format(
				"UPDATE trader t JOIN inventory i ON i.character_id=t.char_id AND i.guid=t.item_sn AND i.item_id=t.item_id "
				"JOIN inventory bag ON bag.character_id=t.char_id AND bag.slot_id={} AND bag.guid={} "
				"SET t.offline_inventory_slot=i.slot_id,t.offline_satchel_slot=bag.slot_id,t.offline_satchel_guid=bag.guid,"
				"t.active_transaction={},"
				"t.offline_parcel_generation=UNHEX(REPLACE(UUID(),'-','')) "
				"WHERE t.id={} AND i.slot_id={} AND t.char_zone_id={} AND t.char_zone_instance_id={} AND t.char_entity_id={} "
				"AND t.active_transaction={} AND t.offline_parcel_generation IS NULL AND "
				"t.item_charges=IF(i.charges=0,1,i.charges) AND i.charges<32767 "
				"AND i.instnodrop=0 AND COALESCE(i.custom_data,'')='' AND i.ornament_icon=0 "
				"AND i.ornament_idfile=0 AND i.ornament_hero_model=0 AND (i.color=0 OR i.color={}) "
				"AND i.augment_one=t.aug_slot_1 AND i.augment_two=t.aug_slot_2 AND i.augment_three=t.aug_slot_3 "
				"AND i.augment_four=t.aug_slot_4 AND i.augment_five=t.aug_slot_5 AND i.augment_six=t.aug_slot_6",
				slot->second.satchel_slot, slot->second.satchel_guid, TraderRepository::OfflineParcel,
				Strings::ToUnsignedBigInt(listing[0]), slot->second.slot,
				Zones::BAZAAR, stall.instance, entity, TraderRepository::CounterOnly, slot->second.catalog_color
			));
			if (updated.RowsAffected() != 1) {
				return {};
			}
		}
		if (!transaction.Commit()) {
			// Keep the native seller from writing either purse or listings if
			// the commit may have reached the database before the disconnect.
			LogError("Bazaar stall registration acknowledgement lost for [{}]", stall.seller);
		}
		return owner;
	} catch (const StorageError &) {
		return {};
	}
}

bool Park(Database &db, std::optional<uint32_t> instance)
{
	const auto storage = CheckStorage(db);
	if (storage != Storage::Ready) {
		return false;
	}
	try {
		Transaction transaction(db);
		const auto scope = instance ? fmt::format(" AND s.instance_id={}", *instance) : std::string{};
		// Phase changes lock the seller before touching any of its listings.
		Query(db, fmt::format(
			"UPDATE bazaar_offline_stalls s SET phase={},owner=UNHEX(REPLACE(UUID(),'-','')) WHERE phase IN ({},{}){}",
			ParkedPhase, Attached, Restored, scope
		));
		Query(db, fmt::format(
			"UPDATE trader t JOIN bazaar_offline_stalls s ON s.character_id=t.char_id AND s.instance_id=t.char_zone_instance_id "
			"SET t.active_transaction={},t.char_entity_id=0 WHERE s.phase={} AND t.char_zone_id={} "
			"AND t.active_transaction={} AND t.offline_inventory_slot IS NOT NULL AND t.offline_parcel_generation IS NOT NULL{}",
			ParkedListing, ParkedPhase, Zones::BAZAAR, TraderRepository::OfflineParcel, scope
		));
		const auto legacy_scope = instance
			? fmt::format(" AND t.char_zone_id={} AND t.char_zone_instance_id={}", Zones::BAZAAR, *instance)
			: std::string{};
		Query(db, fmt::format(
			"DELETE t FROM trader t LEFT JOIN bazaar_offline_stalls s ON s.character_id=t.char_id "
			"AND s.instance_id=t.char_zone_instance_id AND s.phase={} "
			"WHERE (s.character_id IS NULL OR t.char_zone_id<>{} OR t.active_transaction<>{} "
			"OR t.offline_inventory_slot IS NULL OR t.offline_parcel_generation IS NULL){}",
			ParkedPhase, Zones::BAZAAR, ParkedListing, legacy_scope
		));
		return transaction.Commit();
	} catch (const StorageError &) {
		return false;
	}
}

bool Withdraw(Database &db, uint32_t account)
{
	const auto storage = CheckStorage(db);
	if (storage != Storage::Ready) {
		return storage == Storage::Legacy;
	}
	try {
		Transaction transaction(db);
		auto matches = Query(db, fmt::format(
			"SELECT character_id FROM bazaar_offline_stalls WHERE account_id={} FOR UPDATE", account
		));
		if (!matches.RowCount()) {
			return true;
		}
		const auto seller = Strings::ToUnsignedInt(matches.begin()[0]);
		Query(db, fmt::format("DELETE FROM trader WHERE char_id={} AND offline_inventory_slot IS NOT NULL", seller));
		auto removed = Query(db, fmt::format(
			"DELETE FROM bazaar_offline_stalls WHERE account_id={} AND character_id={}", account, seller
		));
		return removed.RowsAffected() == 1 && transaction.Commit();
	} catch (const StorageError &) {
		return false;
	}
}

bool MayDeleteCharacter(Database &db, uint32_t character)
{
	if (!character) {
		return false;
	}
	auto rows = db.QueryDatabase(fmt::format(
		"SELECT 1 FROM bazaar_offline_stalls WHERE character_id={}", character
	), false);
	return rows.Success() ? rows.RowCount() == 0 : rows.ErrorNumber() == ER_NO_SUCH_TABLE;
}

std::vector<Stall> Parked(Database &db, uint32_t instance)
{
	if (CheckStorage(db) != Storage::Ready) {
		return {};
	}
	std::vector<Stall> stalls;
	try {
		auto rows = Query(db, fmt::format(
			"SELECT s.character_id,s.account_id,HEX(s.owner),c.name,s.x,s.y,s.z,s.heading,s.instance_version "
			"FROM bazaar_offline_stalls s JOIN character_data c ON c.id=s.character_id AND c.account_id=s.account_id "
			"WHERE s.instance_id={} AND s.phase={} AND (s.instance_id=0 OR EXISTS "
			"(SELECT 1 FROM instance_list i WHERE i.id=s.instance_id AND i.zone=151 AND i.version=s.instance_version AND i.is_global=1 "
			"AND i.never_expires=1 AND i.offline_bazaar_epoch=s.instance_epoch)) AND EXISTS "
			"(SELECT 1 FROM trader t WHERE t.char_id=s.character_id AND t.active_transaction={}) ORDER BY s.character_id",
			instance, ParkedPhase, ParkedListing
		));
		for (auto row : rows) {
			Stall stall{Strings::ToUnsignedInt(row[0]), Strings::ToUnsignedInt(row[1]), instance, row[2], row[3],
				{std::strtof(row[4], nullptr), std::strtof(row[5], nullptr), std::strtof(row[6], nullptr), std::strtof(row[7], nullptr)},
				Strings::ToUnsignedInt(row[8])};
			if (ValidOwner(stall.owner) && ValidPosition(stall.position)) {
				stalls.push_back(std::move(stall));
			}
		}
	} catch (const StorageError &) {
		return {};
	}
	return stalls;
}

bool Recover(Database &db, const Stall &stall, uint32_t entity)
{
	if (!entity || !ValidOwner(stall.owner)) {
		return false;
	}
	try {
		Transaction transaction(db);
		auto claimed = Query(db, fmt::format(
			"UPDATE bazaar_offline_stalls s SET phase={} WHERE character_id={} AND account_id={} AND instance_id={} "
			"AND owner=UNHEX('{}') AND phase={} AND instance_version={} AND (s.instance_id=0 OR EXISTS "
			"(SELECT 1 FROM instance_list i WHERE i.id=s.instance_id AND i.zone=151 AND i.version=s.instance_version AND i.is_global=1 "
			"AND i.never_expires=1 AND i.offline_bazaar_epoch=s.instance_epoch))",
			Restored, stall.seller, stall.account, stall.instance, stall.owner, ParkedPhase, stall.version
		));
		if (claimed.RowsAffected() != 1) {
			return false;
		}
		auto listings = Query(db, fmt::format(
			"SELECT id FROM trader WHERE char_id={} FOR UPDATE", stall.seller
		));
		if (!listings.RowCount()) {
			return false;
		}
		auto recovered = Query(db, fmt::format(
			"UPDATE trader t JOIN inventory i ON i.character_id=t.char_id AND i.slot_id=t.offline_inventory_slot "
			"AND i.guid=t.item_sn AND i.item_id=t.item_id "
			"JOIN inventory bag ON bag.character_id=t.char_id AND bag.slot_id=t.offline_satchel_slot AND bag.guid=t.offline_satchel_guid "
			"SET t.char_entity_id={},t.active_transaction={},t.offline_parcel_generation=UNHEX(REPLACE(UUID(),'-','')) "
			"WHERE t.char_id={} AND t.char_zone_id={} AND t.char_zone_instance_id={} AND t.active_transaction={} "
			"AND t.char_entity_id=0 AND t.offline_parcel_generation IS NOT NULL AND "
			"t.item_charges=IF(i.charges=0,1,i.charges) AND i.charges<32767",
			entity, TraderRepository::OfflineParcel, stall.seller, Zones::BAZAAR, stall.instance, ParkedListing
		));
		return recovered.RowsAffected() == listings.RowCount() && transaction.Commit();
	} catch (const StorageError &) {
		return false;
	}
}
}
