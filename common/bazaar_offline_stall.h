#ifndef EQEMU_BAZAAR_OFFLINE_STALL_H
#define EQEMU_BAZAAR_OFFLINE_STALL_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

class Database;

namespace OfflineBazaarStall {
struct Position {
	float x;
	float y;
	float z;
	float heading;
};

struct Stall {
	uint32_t seller;
	uint32_t account;
	uint32_t instance;
	std::string owner;
	std::string name;
	Position position;
	uint32_t version = 0;
};

enum class Storage { Ready, Legacy, Unavailable };
Storage CheckStorage(Database &db);

struct ItemLocation {
	uint32_t slot;
	uint32_t satchel_slot;
	uint32_t satchel_guid;
	uint32_t catalog_color;
};

// Captured slots are actual satchel children, keyed by the durable item GUID.
std::optional<std::string> Register(Database &db, const Stall &stall, uint32_t entity,
	const std::map<uint32_t, ItemLocation> &slots);
bool Park(Database &db, std::optional<uint32_t> instance);
bool Withdraw(Database &db, uint32_t account);
bool MayDeleteCharacter(Database &db, uint32_t character);
std::vector<Stall> Parked(Database &db, uint32_t instance);
bool Recover(Database &db, const Stall &stall, uint32_t entity);
}
#endif
