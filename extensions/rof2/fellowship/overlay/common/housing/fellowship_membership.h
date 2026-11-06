#ifndef EQEMU_HOUSING_FELLOWSHIP_MEMBERSHIP_H
#define EQEMU_HOUSING_FELLOWSHIP_MEMBERSHIP_H
#include "access_policy.h"
#include "persistence_service.h"
#include "../database.h"
#include <algorithm>
#include <string>

namespace Housing::FellowshipMembership {
struct Pair { uint32_t actor_id = 0, owner_id = 0; };
namespace Detail {
struct Failure { Error error; };
inline MySQLRequestResult Query(Database& db, const std::string& sql) {
 auto rows = db.QueryDatabase(sql, false);
 if (!rows.Success()) throw Failure{Error::DatabaseFailure};
 return rows;
}
inline uint32_t Number(const char* text) {
 if (!text) throw Failure{Error::DatabaseFailure};
 size_t end = 0; const std::string value(text); auto n = std::stoull(value, &end);
 if (value.empty() || value[0] == '-' || end != value.size() || n >= UINT32_MAX) throw Failure{Error::DatabaseFailure};
 return static_cast<uint32_t>(n);
}
}
// Existing caller owns the SQL transaction and all housing route/lease/owner
// proofs. This adds no lifecycle operations or admissions. The fellowship guard
// is the same row held by create/invite/accept/leave/disband/leader transfer, so
// these membership facts remain current until the caller commits or rolls back.
// The caller already locks actor/owner character rows; repeat their validation
// in deterministic ID order, including account existence and soft deletion.
inline Result<Pair> LoadUnderTransaction(Database& db, uint32_t actor, uint32_t owner) {
 using namespace Detail;
 try {
  if (!AccessPolicy::PositiveIdentity(actor) || !AccessPolicy::PositiveIdentity(owner)) return {{}, Error::InvalidArgument};
  auto transaction = Query(db, "SELECT @@in_transaction");
  if (transaction.RowCount() != 1 || Number(transaction.begin()[0]) != 1) return {{}, Error::InvalidArgument};
  auto tables = Query(db, "SELECT table_name,engine FROM information_schema.tables WHERE table_schema=DATABASE() AND table_name IN ('fellowship_guard','fellowships','fellowship_members')");
  // An older installation without the complete feature keeps fellowship ACLs
  // fail-closed, while ordinary housing authorization remains available.
  if (!tables.RowCount()) return {Pair{}, Error::None};
  if (tables.RowCount() != 3) throw Failure{Error::DatabaseFailure};
  for (auto r = tables.begin(); r.operator!=(tables.end()); ++r) {
   if (!r[1] || std::string(r[1]) != "InnoDB") throw Failure{Error::DatabaseFailure};
  }
  auto guard = Query(db, "SELECT id FROM fellowship_guard WHERE id=1 FOR UPDATE");
  if (guard.RowCount() != 1) throw Failure{Error::DatabaseFailure};
  const auto ids = std::to_string(std::min(actor, owner)) + "," + std::to_string(std::max(actor, owner));
  auto characters = Query(db, "SELECT c.id,c.deleted_at,a.id FROM character_data c LEFT JOIN account a ON a.id=c.account_id WHERE c.id IN (" + ids + ") ORDER BY c.id FOR UPDATE");
  if (characters.RowCount() != (actor == owner ? 1 : 2)) throw Failure{Error::Unauthorized};
  for (auto r = characters.begin(); r.operator!=(characters.end()); ++r) {
   if (r[1] || !r[2] || !Number(r[2])) throw Failure{Error::Unauthorized};
  }
  auto members = Query(db, "SELECT m.character_id,m.fellowship_id,f.id,leader.character_id FROM fellowship_members m LEFT JOIN fellowships f ON f.id=m.fellowship_id LEFT JOIN fellowship_members leader ON leader.character_id=f.leader_id AND leader.fellowship_id=f.id WHERE m.character_id IN (" + ids + ") ORDER BY m.character_id FOR UPDATE");
  if (members.RowCount() > (actor == owner ? 1 : 2)) throw Failure{Error::DatabaseFailure};
  Pair out;
  for (auto r = members.begin(); r.operator!=(members.end()); ++r) {
   const auto character = Number(r[0]), fellowship = Number(r[1]);
   if (!fellowship || !r[2] || Number(r[2]) != fellowship || !r[3] || !Number(r[3])) throw Failure{Error::DatabaseFailure};
   if (character == actor) out.actor_id = fellowship;
   if (character == owner) out.owner_id = fellowship;
  }
  return {out, Error::None};
 } catch (const Failure& failure) { return {{}, failure.error}; }
 catch (...) { return {{}, Error::DatabaseFailure}; }
}
// A personal plot follows its current character owner's fellowship. Guild plots
// have no proven character association: never borrow the guild leader's group.
inline Error PopulateUnderTransaction(Database& db, AccessPolicy::TrustedActor& actor,
 AccessPolicy::ResolvedPlot& plot) {
 actor.fellowship_id = plot.fellowship_id = 0;
 if (plot.type != AccessPolicy::PlotType::Personal) return Error::None;
 auto membership = LoadUnderTransaction(db, actor.character_id, plot.owner_character_id);
 if (!membership) return membership.error;
 actor.fellowship_id = membership.value.actor_id;
 plot.fellowship_id = membership.value.owner_id;
 return Error::None;
}
}
#endif
