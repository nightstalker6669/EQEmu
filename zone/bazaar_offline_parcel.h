#ifndef EQEMU_BAZAAR_OFFLINE_PARCEL_H
#define EQEMU_BAZAAR_OFFLINE_PARCEL_H

#include "common/housing/coin_wallet.h"

#include <cstdint>
#include <optional>

class Database;

namespace OfflineBazaar {
using Coins = Housing::CoinWallet::Snapshot;

constexpr uint32_t FirstQuote = 1U << 31;

bool HasTransactionalStorage(Database &db);
bool Activate(Database &db, uint32_t seller);
bool SetParcelAvailability(Database &db, uint32_t seller, bool available);
std::optional<Coins> ReadWallet(Database &db, uint32_t character_id);
uint32_t Quote(Database &db, uint64_t listing_id, uint32_t price, int32_t stock);

struct Purchase {
	uint32_t token;
	uint32_t buyer;
	uint32_t buyer_zone;
	uint32_t buyer_instance;
	uint32_t wire_trader;
	uint32_t item;
	uint32_t price;
	uint32_t quantity;
	bool stackable;
	bool parcelable;
	uint32_t catalog_color;
	Coins carried;
	double fee_rate;
	int32_t mailbox_limit;
};

enum class Status { Stale, Failed, Funds, MailboxFull, Committed, Replay, CommitUnknown };

struct Outcome {
	Status status;
	uint32_t item = 0;
	uint32_t quantity = 0;
	uint32_t slot = 0;
	uint64_t fee = 0;
	Coins carried{};
};

Outcome Settle(Database &db, const Purchase &request);
bool ResolveInspect(Database &db, uint32_t token, uint32_t &trader, uint32_t &serial);
}

#endif
