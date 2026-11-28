#include "bazaar_offline_parcel.h"

#include "common/database.h"
#include "common/eq_constants.h"
#include "common/repositories/character_parcels_repository.h"
#include "common/repositories/trader_repository.h"
#include "common/strings.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace OfflineBazaar {
namespace {
constexpr uint8_t OfflineParcel = TraderRepository::OfflineParcel;
constexpr uint64_t MaximumPrice = 2'000'000'000;

struct DatabaseFailure {};

MySQLRequestResult Query(Database &db, const std::string &sql)
{
	auto rows = db.QueryDatabase(sql, false);
	if (!rows.Success()) {
		throw DatabaseFailure{};
	}
	return rows;
}

class Transaction {
public:
	explicit Transaction(Database &db) : db_(db)
	{
		if (!db_.TransactionBeginStrict().Success()) {
			throw DatabaseFailure{};
		}
	}

	~Transaction()
	{
		if (active_) {
			db_.TransactionRollbackStrict();
		}
	}

	bool Commit()
	{
		active_ = false;
		return db_.TransactionCommitStrict().Success();
	}

private:
	Database &db_;
	bool active_ = true;
};

std::optional<Coins> Wallet(Database &db, uint32_t character_id, bool locked)
{
	auto rows = Query(db, fmt::format(
		"SELECT copper,silver,gold,platinum FROM character_currency WHERE id={}{}",
		character_id, locked ? " FOR UPDATE" : ""
	));
	if (rows.RowCount() != 1) {
		return {};
	}
	auto row = rows.begin();
	const auto copper = Strings::ToUnsignedInt(row[0]);
	const auto silver = Strings::ToUnsignedInt(row[1]);
	const auto gold = Strings::ToUnsignedInt(row[2]);
	const auto platinum = Strings::ToUnsignedInt(row[3]);
	if (std::max({copper, silver, gold, platinum}) > INT32_MAX) {
		return {};
	}
	return Coins{int32_t(copper), int32_t(silver), int32_t(gold), int32_t(platinum)};
}

std::optional<Coins> Credit(const Coins &coins, uint64_t price)
{
	const auto total = Housing::CoinWallet::TotalCopper(coins).value + price;
	if (total / 1000 > INT32_MAX) {
		return {};
	}
	return Coins{int32_t(total % 10), int32_t(total / 10 % 10), int32_t(total / 100 % 10), int32_t(total / 1000)};
}

void RequireOne(MySQLRequestResult result)
{
	if (result.RowsAffected() != 1) {
		throw DatabaseFailure{};
	}
}

void WriteWallet(Database &db, uint32_t character_id, const Coins &coins)
{
	RequireOne(Query(db, fmt::format(
		"UPDATE character_currency SET copper={},silver={},gold={},platinum={} WHERE id={}",
		coins.copper, coins.silver, coins.gold, coins.platinum, character_id
	)));
}

bool SameTrader(uint32_t wire_id, uint32_t seller, uint32_t instance)
{
	return wire_id == seller || wire_id == TraderRepository::TRADER_CONVERT_ID + instance;
}

uint32_t FreeMailboxSlot(Database &db, uint32_t character_id, int32_t limit)
{
	auto rows = Query(db, fmt::format(
		"SELECT slot_id FROM character_parcels WHERE char_id={} ORDER BY slot_id FOR UPDATE", character_id
	));
	if (limit < PARCEL_BEGIN_SLOT || rows.RowCount() >= static_cast<uint64_t>(limit)) {
		return 0;
	}
	uint32_t slot = PARCEL_BEGIN_SLOT;
	for (auto row : rows) {
		const auto used = Strings::ToUnsignedInt(row[0]);
		if (used > slot) {
			break;
		}
		if (used == slot) {
			++slot;
		}
	}
	return slot <= static_cast<uint32_t>(limit) ? slot : 0;
}
} // namespace

bool HasTransactionalStorage(Database &db)
{
	auto result = db.QueryDatabase(std::string(
		"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
		"AND table_name IN ('bazaar_offline_parcel_quotes','trader','inventory','character_currency','character_parcels')"
	), false);
	return result.Success() && result.RowCount() == 1 && Strings::ToUnsignedInt(result.begin()[0]) == 5;
}

bool Activate(Database &db, uint32_t seller)
{
	if (!HasTransactionalStorage(db)) {
		return false;
	}
	return db.QueryDatabase(fmt::format(
		"UPDATE trader SET active_transaction={},offline_parcel_generation=UNHEX(REPLACE(UUID(),'-','')) "
		"WHERE char_id={} AND active_transaction={} AND offline_parcel_generation IS NULL",
		OfflineParcel, seller, TraderRepository::CounterOnly
	), false).Success();
}

bool SetParcelAvailability(Database &db, uint32_t seller, bool available)
{
	const auto before = available ? TraderRepository::CounterOnly : OfflineParcel;
	const auto after = available ? OfflineParcel : TraderRepository::CounterOnly;
	return db.QueryDatabase(fmt::format(
		"UPDATE trader SET active_transaction={} WHERE char_id={} AND active_transaction={} AND offline_parcel_generation IS NOT NULL", after, seller, before
	), false).Success();
}

std::optional<Coins> ReadWallet(Database &db, uint32_t character_id)
{
	try {
		return Wallet(db, character_id, false);
	} catch (const DatabaseFailure &) {
		return {};
	}
}

uint32_t Quote(Database &db, uint64_t listing_id, uint32_t price, int32_t stock)
{
	const auto predicate = fmt::format(
		"t.id={} AND t.active_transaction={} AND t.item_cost={} AND t.item_charges={}",
		listing_id, OfflineParcel, price, stock
	);
	const auto lookup = fmt::format(
		"SELECT q.token FROM trader t JOIN bazaar_offline_parcel_quotes q "
		"ON q.generation=t.offline_parcel_generation AND q.stock=t.item_charges "
		"AND q.unit_price=t.item_cost WHERE {}", predicate
	);
	auto existing = db.QueryDatabase(lookup, false);
	if (!existing.Success()) {
		return 0;
	}
	if (!existing.RowCount()) {
		auto inserted = db.QueryDatabase(fmt::format(
			"INSERT INTO bazaar_offline_parcel_quotes (listing_id,generation,seller_id,serial,instance_id,item_id,unit_price,stock) "
			"SELECT t.id,t.offline_parcel_generation,t.char_id,t.item_sn,t.char_zone_instance_id,t.item_id,t.item_cost,t.item_charges "
			"FROM trader t WHERE {} ON DUPLICATE KEY UPDATE token=token", predicate
		), false);
		if (!inserted.Success()) {
			return 0;
		}
		existing = db.QueryDatabase(lookup, false);
	}
	const auto token = existing.Success() && existing.RowCount() == 1
		? Strings::ToUnsignedInt(existing.begin()[0]) : 0;
	return token >= FirstQuote ? token : 0;
}

bool ResolveInspect(Database &db, uint32_t token, uint32_t &trader, uint32_t &serial)
{
	auto rows = db.QueryDatabase(fmt::format(
		"SELECT q.seller_id,q.serial,q.instance_id FROM bazaar_offline_parcel_quotes q "
		"JOIN trader t ON t.id=q.listing_id AND t.char_id=q.seller_id AND t.item_sn=q.serial "
		"AND t.item_cost=q.unit_price AND t.item_charges=q.stock AND t.offline_parcel_generation=q.generation "
		"WHERE q.token={} AND q.buyer_id=0",
		token
	), false);
	if (!rows.Success() || rows.RowCount() != 1) {
		return false;
	}
	auto row = rows.begin();
	const auto seller = Strings::ToUnsignedInt(row[0]);
	if (!SameTrader(trader, seller, Strings::ToUnsignedInt(row[2]))) {
		return false;
	}
	trader = seller;
	serial = Strings::ToUnsignedInt(row[1]);
	return true;
}

Outcome Settle(Database &db, const Purchase &request)
{
	if (request.token < FirstQuote || !request.buyer || !request.price || !request.quantity ||
		!std::isfinite(request.fee_rate) || request.fee_rate < 0) {
		return {Status::Stale};
	}

	try {
		Transaction transaction(db);
		// Search locks the listing before recording its quote. Use the same order.
		auto binding = Query(db, fmt::format(
			"SELECT listing_id FROM bazaar_offline_parcel_quotes WHERE token={}", request.token
		));
		if (binding.RowCount() != 1) {
			return {Status::Stale};
		}
		const auto listing_id = Strings::ToUnsignedBigInt(binding.begin()[0]);
		auto listing = Query(db, fmt::format(
			"SELECT char_id,item_sn,item_id,item_cost,item_charges,active_transaction,"
			"aug_slot_1,aug_slot_2,aug_slot_3,aug_slot_4,aug_slot_5,aug_slot_6,HEX(offline_parcel_generation),"
			"char_zone_id,char_zone_instance_id,offline_inventory_slot,offline_satchel_slot,offline_satchel_guid "
			"FROM trader WHERE id={} FOR UPDATE",
			listing_id
		));
		auto quotes = Query(db, fmt::format(
			"SELECT seller_id,serial,instance_id,item_id,unit_price,stock,buyer_id,requested_quantity,"
			"sold_quantity,HEX(generation),delivery_slot,delivery_serial FROM bazaar_offline_parcel_quotes WHERE token={} FOR UPDATE", request.token
		));
		if (quotes.RowCount() != 1) {
			return {Status::Stale};
		}
		auto quote = quotes.begin();
		const auto seller = Strings::ToUnsignedInt(quote[0]);
		const auto serial = Strings::ToUnsignedInt(quote[1]);
		const auto instance = Strings::ToUnsignedInt(quote[2]);
		const auto item = Strings::ToUnsignedInt(quote[3]);
		const auto price = Strings::ToUnsignedInt(quote[4]);
		const auto stock = Strings::ToInt(quote[5]);
		const auto buyer = Strings::ToUnsignedInt(quote[6]);
		if (!seller || request.buyer == seller || request.price != price || request.item != item ||
			!SameTrader(request.wire_trader, seller, instance)) {
			return {Status::Stale};
		}
		if (buyer) {
			if (buyer != request.buyer || request.quantity != Strings::ToUnsignedInt(quote[7]) ||
				request.counter.has_value() != (quote[10] != nullptr)) {
				return {Status::Stale};
			}
			return {Status::Replay, item, Strings::ToUnsignedInt(quote[8])};
		}
		if (listing.RowCount() != 1) {
			return {Status::Stale};
		}
		auto sale = listing.begin();
		if (Strings::ToUnsignedInt(sale[0]) != seller || Strings::ToUnsignedInt(sale[1]) != serial ||
			Strings::ToUnsignedInt(sale[2]) != item || Strings::ToUnsignedInt(sale[3]) != price ||
			Strings::ToInt(sale[4]) != stock || Strings::ToUnsignedInt(sale[5]) != OfflineParcel ||
			!sale[12] || !quote[9] || std::string_view(sale[12]) != quote[9] || !request.parcelable ||
			Strings::ToUnsignedInt(sale[13]) != Zones::BAZAAR || Strings::ToUnsignedInt(sale[14]) != instance ||
			(request.counter.has_value() !=
				(request.buyer_zone == Zones::BAZAAR && request.buyer_instance == instance)) ||
			(request.counter && (!sale[15] || !request.counter->slot || !request.counter->serial ||
				request.counter->serial > INT32_MAX || request.fee_rate != 0))) {
			return {Status::Stale};
		}
		if ((request.stackable && (stock <= 0 || request.quantity > static_cast<uint32_t>(stock))) ||
			(!request.stackable && request.quantity != 1) || request.quantity > INT16_MAX) {
			return {Status::Stale};
		}
		const auto cost = uint64_t(price) * request.quantity;
		const auto fee = std::round(double(cost) * request.fee_rate);
		if (cost > MaximumPrice || fee > MaximumPrice || fee + cost > MaximumPrice) {
			return {Status::Stale};
		}

		auto items = Query(db, fmt::format(
			"SELECT slot_id,charges,augment_one,augment_two,augment_three,augment_four,augment_five,augment_six "
			"FROM inventory WHERE character_id={} AND guid={} AND item_id={} AND instnodrop=0 "
			"AND COALESCE(custom_data,'')='' AND ornament_icon=0 AND ornament_idfile=0 AND ornament_hero_model=0 "
			"AND (color=0 OR color={}) FOR UPDATE", seller, serial, item, request.catalog_color
		));
		if (items.RowCount() != 1) {
			return {Status::Stale};
		}
		auto inventory = items.begin();
		const auto slot = Strings::ToUnsignedInt(inventory[0]);
		if (sale[15]) {
			if (Strings::ToUnsignedInt(sale[15]) != slot || !sale[16] || !sale[17]) {
				return {Status::Stale};
			}
			auto authority = Query(db, fmt::format(
				"SELECT 1 FROM bazaar_offline_stalls s LEFT JOIN instance_list i ON i.id=s.instance_id "
				"AND i.zone={} AND i.version=s.instance_version AND i.is_global=1 AND i.never_expires=1 "
				"AND i.offline_bazaar_epoch=s.instance_epoch WHERE s.character_id={} AND s.instance_id={} "
				"AND s.phase IN (1,3) AND (s.instance_id=0 OR i.id IS NOT NULL) LOCK IN SHARE MODE",
				Zones::BAZAAR, seller, instance
			));
			if (authority.RowCount() != 1) {
				return {Status::Stale};
			}
			auto satchel = Query(db, fmt::format(
				"SELECT slot_id FROM inventory WHERE character_id={} AND slot_id={} AND guid={} FOR UPDATE",
				seller, Strings::ToUnsignedInt(sale[16]), Strings::ToUnsignedInt(sale[17])
			));
			if (satchel.RowCount() != 1) {
				return {Status::Stale};
			}
		}
		const auto charges = static_cast<int16_t>(Strings::ToUnsignedInt(inventory[1]));
		// Native inventory uses 32767 for unlimited/negative charges.
		if (charges < 0 || charges == INT16_MAX || (charges ? charges : 1) != stock) {
			return {Status::Stale};
		}
		for (int augment = 0; augment < 6; ++augment) {
			if (Strings::ToUnsignedInt(inventory[augment + 2]) != Strings::ToUnsignedInt(sale[augment + 6])) {
				return {Status::Stale};
			}
		}

		const auto first = Wallet(db, std::min(seller, request.buyer), true);
		const auto second = Wallet(db, std::max(seller, request.buyer), true);
		if (!first || !second) {
			return {Status::Failed};
		}
		const auto &buyer_wallet = request.buyer < seller ? *first : *second;
		const auto &seller_wallet = request.buyer < seller ? *second : *first;
		if (!(buyer_wallet == request.carried)) {
			return {Status::Stale};
		}
		const auto buyer_after = Housing::CoinWallet::DebitCarried(buyer_wallet, cost + static_cast<uint64_t>(fee));
		const auto seller_after = Credit(seller_wallet, cost);
		if (!buyer_after || !seller_after) {
			return {Status::Funds};
		}
		uint32_t parcel_slot = 0;
		uint32_t parcel_id = 0;
		if (request.counter) {
			// The native caller preflights a free slot and creates a separate item
			// instance. The primary key guards the durable slot at insertion.
			auto serials = Query(db, fmt::format(
				"SELECT slot_id FROM inventory WHERE character_id={} AND guid={} FOR UPDATE",
				request.buyer, request.counter->serial
			));
			if (serials.RowCount()) {
				return {Status::Stale};
			}
			RequireOne(Query(db, fmt::format(
				"INSERT INTO inventory (character_id,slot_id,item_id,charges,color,augment_one,augment_two,"
				"augment_three,augment_four,augment_five,augment_six,instnodrop,custom_data,ornament_icon,"
				"ornament_idfile,ornament_hero_model,guid) "
				"SELECT {},{},item_id,{},color,augment_one,augment_two,augment_three,augment_four,"
				"augment_five,augment_six,instnodrop,custom_data,ornament_icon,ornament_idfile,ornament_hero_model,{} "
				"FROM inventory WHERE character_id={} AND slot_id={} AND guid={}",
				request.buyer, request.counter->slot, request.stackable ? request.quantity : charges,
				request.counter->serial, seller, slot, serial
			)));
		} else {
			parcel_slot = FreeMailboxSlot(db, request.buyer, request.mailbox_limit);
			if (!parcel_slot) {
				return {Status::MailboxFull};
			}
			auto names = Query(db, fmt::format("SELECT name FROM character_data WHERE id={}", seller));
			if (names.RowCount() != 1 || !names.begin()[0]) {
				return {Status::Stale};
			}

			CharacterParcelsRepository::CharacterParcels parcel{};
			parcel.char_id = request.buyer;
			parcel.item_id = item;
			parcel.aug_slot_1 = Strings::ToUnsignedInt(sale[6]);
			parcel.aug_slot_2 = Strings::ToUnsignedInt(sale[7]);
			parcel.aug_slot_3 = Strings::ToUnsignedInt(sale[8]);
			parcel.aug_slot_4 = Strings::ToUnsignedInt(sale[9]);
			parcel.aug_slot_5 = Strings::ToUnsignedInt(sale[10]);
			parcel.aug_slot_6 = Strings::ToUnsignedInt(sale[11]);
			parcel.slot_id = parcel_slot;
			parcel.quantity = request.stackable ? request.quantity : std::max<int16_t>(charges, 1);
			parcel.from_name = names.begin()[0];
			parcel.note = "Delivered from a Bazaar Purchase";
			parcel.sent_date = time(nullptr);
			const auto saved = CharacterParcelsRepository::InsertOne(db, parcel);
			if (!saved.id) {
				return {Status::Failed};
			}

			parcel_id = saved.id;
		}

		WriteWallet(db, request.buyer, buyer_after.value.after);
		WriteWallet(db, seller, *seller_after);
		const auto remaining = request.stackable ? stock - static_cast<int32_t>(request.quantity) : 0;
		const auto item_where = fmt::format(" WHERE character_id={} AND slot_id={} AND guid={}", seller, slot, serial);
		if (remaining) {
			RequireOne(Query(db, fmt::format("UPDATE inventory SET charges={}{}", remaining, item_where)));
			RequireOne(Query(db, fmt::format("UPDATE trader SET item_charges={},listing_date=NOW() WHERE id={}", remaining, listing_id)));
		} else {
			RequireOne(Query(db, "DELETE FROM inventory" + item_where));
			RequireOne(Query(db, fmt::format("DELETE FROM trader WHERE id={}", listing_id)));
		}
		RequireOne(Query(db, fmt::format(
			"UPDATE bazaar_offline_parcel_quotes SET buyer_id={},requested_quantity={},sold_quantity={},parcel_id={},fee={},"
			"delivery_slot={},delivery_serial={} WHERE token={} AND buyer_id=0",
			request.buyer, request.quantity, request.quantity, parcel_id, static_cast<uint64_t>(fee),
			request.counter ? std::to_string(request.counter->slot) : "NULL",
			request.counter ? request.counter->serial : 0, request.token
		)));
		if (!transaction.Commit()) {
			return {Status::CommitUnknown};
		}
		return {Status::Committed, item, request.quantity, request.counter ? request.counter->slot : parcel_slot, static_cast<uint64_t>(fee), buyer_after.value.after};
	} catch (const DatabaseFailure &) {
		return {Status::Failed};
	}
}
} // namespace OfflineBazaar
