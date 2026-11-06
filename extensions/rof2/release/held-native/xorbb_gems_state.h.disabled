#pragma once

// Fixed task 3040490 persistence only. This is NOT a terminal award barrier.
#include "common/database.h"
#include "common/strings.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <map>
#include <string>
#include <vector>

namespace XorbbGemsState {
constexpr uint32_t Task = 3040490;
constexpr std::array<int, 6> Waves{{75, 65, 55, 45, 35, 25}};
constexpr std::array<const char*, 4> Flags{{
	"participant_death", "trapped_open", "contestant_killed", "contestant_attribution_unknown"
}};

struct Result {
	bool ok = false;
	std::string status = "invalid_request";
	std::map<std::string, std::string> values;
};

inline bool Positive(const std::string& text, int64_t& number)
{
	if (text.empty() || text.front() < '1' || text.front() > '9') { return false; }
	auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
	return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() && number > 0;
}

// All callers share the active occurrence row lock. Never use DataBucket::SetData
// for these keys; it bypasses this lock and discards write errors.
inline Result Apply(Database& db, int64_t occurrence, uint32_t character,
	uint16_t instance, const std::string& op, const std::string& field,
	const std::string& value)
{
	Result out;
	if (occurrence <= 0) { return out; }
	const bool chrom = op.compare(0, 6, "chrom_") == 0;
	const bool flag = std::find(Flags.begin(), Flags.end(), field) != Flags.end();
	const bool initialize = op == "initialize";
	const bool fail = op == "fail";
	const bool bind = op == "bind";
	const bool read = op == "read" || op == "chrom_read";
	const bool ci = op == "chrom_initialize";
	const bool reserve = op == "chrom_reserve";
	const bool slot = op == "chrom_slot";
	const bool finish = op == "chrom_done";
	if (!(initialize || fail || bind || read || ci || reserve || slot || finish) ||
		(fail && !flag) || ((chrom || bind) && instance == 0)) { return out; }
	std::string wave;
	for (auto hp : Waves) {
		const auto w = "wave" + std::to_string(hp);
		if ((reserve || finish) && field == w) { wave = w; }
		if (slot && (field == w + "_slot1" || field == w + "_slot2")) { wave = w; }
	}
	int64_t entity = 0;
	if (((reserve || finish || slot) && wave.empty()) ||
		(slot && (!Positive(value, entity) || entity > UINT16_MAX))) { return out; }
	if ((initialize || bind || ci) && character == 0) { return out; }
	auto active = db.QueryDatabase("SELECT @@in_transaction");
	if (!active.Success() || active.RowCount() != 1 ||
		!active.begin()[0] || std::string(active.begin()[0]) != "0") {
		out.status = "query_error_or_nested_transaction"; return out;
	}
	const auto engines = db.QueryDatabase("SELECT TABLE_NAME FROM information_schema.TABLES "
		"WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN ('shared_tasks','data_buckets') AND ENGINE='InnoDB'");
	if (!engines.Success() || engines.RowCount() != 2) { out.status = "transactional_tables_required"; return out; }
	if (!db.TransactionBeginStrict().Success()) { out.status = "begin_failed"; return out; }
	struct Rollback {
		Database& db; bool committed = false;
		~Rollback() { if (!committed) { db.TransactionRollbackStrict(); } }
	} transaction{db};
	auto task = db.QueryDatabase(fmt::format(
		"SELECT id FROM shared_tasks WHERE id={} AND task_id={} AND completion_time IS NULL "
		"AND (expire_time IS NULL OR expire_time>NOW()) FOR UPDATE", occurrence, Task));
	if (!task.Success() || task.RowCount() != 1) { out.status = "occurrence_missing_or_query_error"; return out; }
	if (character) {
		const auto member = db.QueryDatabase(fmt::format(
			"SELECT character_id FROM shared_task_members WHERE shared_task_id={} AND character_id={}", occurrence, character));
		if (!member.Success() || member.RowCount() != 1) { out.status = "membership_missing_or_query_error"; return out; }
	}
	if (instance) {
		const auto mapping = db.QueryDatabase(fmt::format(
			"SELECT dz.id FROM shared_task_dynamic_zones m JOIN dynamic_zones dz ON dz.id=m.dynamic_zone_id "
			"JOIN instance_list i ON i.id=dz.instance_id WHERE m.shared_task_id={} AND i.id={} "
			"AND i.zone=753 AND i.version=3 AND (i.never_expires=1 OR i.start_time+i.duration>UNIX_TIMESTAMP())",
			occurrence, instance));
		if (!mapping.Success() || mapping.RowCount() != 1) { out.status = "instance_missing_or_query_error"; return out; }
	}
	const auto gems = "rof2_xorbb_gems_" + std::to_string(occurrence) + "_";
	const auto cp = "rof2_xorbb_chrom_" + std::to_string(occurrence) + "_" + std::to_string(instance) + "_";
	std::vector<std::string> keys{gems + "monitored", gems + "instance"};
	for (auto f : Flags) { keys.push_back(gems + f); }
	if (chrom) {
		keys.push_back(cp + "initialized");
		for (auto hp : Waves) {
			const auto w = cp + "wave" + std::to_string(hp);
			keys.insert(keys.end(), {w, w + "_slot1", w + "_slot2"});
		}
	}
	std::string quoted;
	for (const auto& key : keys) { if (!quoted.empty()) { quoted += ','; } quoted += "'" + key + "'"; }
	auto buckets = db.QueryDatabase("SELECT `key`,`value`,expires FROM data_buckets WHERE "
		"account_id=0 AND character_id=0 AND npc_id=0 AND bot_id=0 AND zone_id=0 AND instance_id=0 AND `key` IN (" + quoted + ") FOR UPDATE");
	if (!buckets.Success()) { out.status = "read_failed"; return out; }
	std::map<std::string, std::string> values;
	for (auto row : buckets) {
		if (!row[0] || !row[1] || !row[2] || std::string(row[2]) != "0" || !values.emplace(row[0], row[1]).second) {
			out.status = "duplicate_expiring_or_invalid_state"; return out;
		}
	}
	for (auto f : Flags) {
		const auto it = values.find(gems + f);
		if (it != values.end() && it->second != "0" && it->second != "1") { out.status = "invalid_flag"; return out; }
	}
	for (const auto& marker : {gems + "monitored", cp + "initialized"}) {
		if (values.count(marker) && values.at(marker) != "1") { out.status = "invalid_marker"; return out; }
	}
	if (values.count(gems + "instance")) {
		int64_t stored = 0;
		if (!Positive(values.at(gems + "instance"), stored) || stored > UINT16_MAX ||
			(instance && stored != instance)) { out.status = "immutable_instance_mismatch"; return out; }
	}
	if (instance && !bind && !values.count(gems + "instance")) { out.status = "instance_not_bound"; return out; }
	auto put = [&](const std::string& key, const std::string& text) {
		const bool exists = values.count(key);
		if (exists && values.at(key) == text) { return true; }
		const auto sql = exists ? fmt::format("UPDATE data_buckets SET `value`='{}' WHERE "
			"account_id=0 AND character_id=0 AND npc_id=0 AND bot_id=0 AND zone_id=0 AND instance_id=0 AND `key`='{}'",
			Strings::Escape(text), key) : fmt::format("INSERT INTO data_buckets "
			"(`key`,`value`,expires,account_id,character_id,npc_id,bot_id,zone_id,instance_id) VALUES ('{}','{}',0,0,0,0,0,0,0)", key, Strings::Escape(text));
		const auto write = db.QueryDatabase(sql);
		if (!write.Success() || write.RowsAffected() != 1) { return false; }
		values[key] = text; return true;
	};
	bool changed = true;
	if (initialize) {
		if (values.count(gems + "monitored")) {
			for (auto f : Flags) { if (!values.count(gems + f)) { out.status = "incomplete_initialized_state"; return out; } }
		} else {
			for (auto f : Flags) { if (!values.count(gems + f)) { changed = changed && put(gems + f, "0"); } }
			changed = changed && put(gems + "monitored", "1");
		}
	} else if (fail) { changed = put(gems + field, "1"); }
	else if (bind) { changed = put(gems + "instance", std::to_string(instance)); }
	else {
		if (!values.count(gems + "monitored") || !values.count(gems + "instance")) { out.status = "missing"; return out; }
		for (auto f : Flags) { if (!values.count(gems + f)) { out.status = "missing"; return out; } }
		if (chrom) {
			for (auto hp : Waves) {
				const auto w = cp + "wave" + std::to_string(hp);
				if (values.count(w) && values.at(w) != "armed" && values.at(w) != "pending" && values.at(w) != "done") {
					out.status = "invalid_wave"; return out;
				}
				for (auto n : {1, 2}) {
					const auto k = w + "_slot" + std::to_string(n); int64_t recorded = 0;
					if (values.count(k) && (!Positive(values.at(k), recorded) || recorded > UINT16_MAX)) { out.status = "invalid_slot"; return out; }
				}
				if (values.count(w) && values.at(w) == "armed" &&
					(values.count(w + "_slot1") || values.count(w + "_slot2"))) { out.status = "armed_with_slots"; return out; }
				if (values.count(w) && values.at(w) == "done" &&
					(!values.count(w + "_slot1") || !values.count(w + "_slot2") ||
					values.at(w + "_slot1") == values.at(w + "_slot2"))) { out.status = "incomplete_done_wave"; return out; }
			}
			if (ci) {
				for (const auto& entry : values) { if (entry.first.compare(0, cp.size(), cp) == 0) { out.status = "existing_or_partial_wave_state"; return out; } }
				for (auto hp : Waves) { changed = changed && put(cp + "wave" + std::to_string(hp), "armed"); }
				changed = changed && put(cp + "initialized", "1");
			} else {
				if (!values.count(cp + "initialized")) { out.status = "missing"; return out; }
				for (auto hp : Waves) { if (!values.count(cp + "wave" + std::to_string(hp))) { out.status = "missing"; return out; } }
				const auto k = cp + wave;
				if (reserve) {
					if (values.at(k) != "armed" || values.count(k + "_slot1") || values.count(k + "_slot2")) { out.status = "not_armed"; return out; }
					changed = put(k, "pending");
				} else if (slot || finish) {
					if (values.at(k) != "pending") { out.status = "not_pending"; return out; }
					if (slot) {
						if (values.count(cp + field)) { out.status = "slot_already_recorded"; return out; }
						changed = put(cp + field, value);
					} else {
						if (!values.count(k + "_slot1") || !values.count(k + "_slot2") || values.at(k + "_slot1") == values.at(k + "_slot2")) { out.status = "incomplete_slots"; return out; }
						changed = put(k, "done");
					}
				}
			}
		}
	}
	if (!changed) { out.status = "write_failed"; return out; }
	// Read the complete exact postimage while still holding the occurrence lock.
	auto post = db.QueryDatabase("SELECT `key`,`value`,expires FROM data_buckets WHERE "
		"account_id=0 AND character_id=0 AND npc_id=0 AND bot_id=0 AND zone_id=0 AND instance_id=0 AND `key` IN (" + quoted + ")");
	std::map<std::string, std::string> actual;
	if (!post.Success()) { out.status = "postimage_read_failed"; return out; }
	for (auto row : post) {
		if (!row[0] || !row[1] || !row[2] || std::string(row[2]) != "0" || !actual.emplace(row[0], row[1]).second) { out.status = "postimage_invalid"; return out; }
	}
	if (actual != values) { out.status = "postimage_mismatch"; return out; }
	if (!db.TransactionCommitStrict().Success()) { out.status = "commit_uncertain"; return out; }
	transaction.committed = true;
	for (const auto& entry : values) {
		const auto& prefix = chrom && entry.first.compare(0, cp.size(), cp) == 0 ? cp : gems;
		out.values[entry.first.substr(prefix.size())] = entry.second;
	}
	out.ok = true; out.status = "committed"; return out;
}
} // namespace XorbbGemsState
