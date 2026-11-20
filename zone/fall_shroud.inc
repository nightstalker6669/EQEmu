// Included by client_spirit_shroud.cpp to share its checkpoint, snapshot and actor lock.
namespace {
constexpr uint32_t FallTask = 920243;
constexpr uint32_t FallZone = 348;
constexpr uint16_t FallVersion = 1;
constexpr uint32_t FallReturnZone = 362;
constexpr uint32_t FallStillpool = 362104;
constexpr uint32_t FallStillpoolSpawn = 102601;
constexpr uint32_t FallDraygun = 348005;
constexpr uint32_t FallDraygunSpawn = 260192;
constexpr uint32_t FallShard = 64048;
constexpr uint8_t FallInventoryPolicy = 2;
constexpr char FallCorpseToken[] = "fall_shroud_commander";

bool FallClass(uint64_t class_id)
{
 return class_id == 1 || class_id == 2 || class_id == 9 || class_id == 12 || class_id == 14;
}

struct FallLocation {
 uint32_t zone;
 uint32_t instance;
 float x;
 float y;
 float z;
 float heading;
};

float FallCoord(const char* value)
{
 if (!value) throw std::runtime_error("missing Fall location");
 size_t end = 0;
 const auto coordinate = std::stof(value, &end);
 if (value[end] || !std::isfinite(coordinate)) throw std::runtime_error("invalid Fall location");
 return coordinate;
}

template <typename Row> FallLocation FallPosition(uint32_t z, uint32_t instance, Row row, size_t offset)
{
 return {z, instance, FallCoord(row[offset]), FallCoord(row[offset + 1]),
  FallCoord(row[offset + 2]), FallCoord(row[offset + 3])};
}

std::string FallLocationSet(const FallLocation& loc, const std::string& prefix = {})
{
 return prefix + "zone_id=" + S(loc.zone) + "," + prefix + "zone_instance=" + S(loc.instance) +
  "," + prefix + "x=" + std::to_string(loc.x) + "," + prefix + "y=" + std::to_string(loc.y) +
  "," + prefix + "z=" + std::to_string(loc.z) + "," + prefix + "heading=" + std::to_string(loc.heading);
}

struct FallSettings {
 uint32_t commander;
 FallLocation respawn;
 uint32_t refill;
 uint32_t normal_bp;
 uint32_t aa_bp;
};

FallSettings ReadFallSettings(Database& db)
{
 auto rows = Q(db, "SELECT commander_type,respawn_x,respawn_y,respawn_z,respawn_heading,"
  "refill_percent,normal_xp_bp,aa_xp_bp FROM fall_shroud_config WHERE id=1 AND enabled=1 FOR UPDATE");
 if (rows.RowCount() != 1) throw std::runtime_error("Fall shrouds are disabled");
 auto row = rows.begin();
 const auto commander = N(row[0]), refill = N(row[5]), normal = N(row[6]), aa = N(row[7]);
 if (!commander || commander > UINT32_MAX || refill < 1 || refill > 100 || normal > 10000 || aa > 10000) {
  throw std::runtime_error("invalid Fall settings");
 }
 return {uint32_t(commander), FallPosition(FallZone, 0, row, 1), uint32_t(refill), uint32_t(normal), uint32_t(aa)};
}

// This distinct connection is never retried. Destroying it releases an
// uncommitted transaction, including after a failure in a custody check.
struct FallTransaction {
 ZoneDatabase db;
 uint64_t connection = 0;

 explicit FallTransaction(Client& client)
 {
  auto* config = EQEmuConfig::get();
  if (!config || !db.Connect(config->DatabaseHost, config->DatabaseUsername,
      config->DatabasePassword, config->DatabaseDB, config->DatabasePort, "fall-shroud")) {
   throw std::runtime_error("Fall database unavailable");
  }
  Q(db, "START TRANSACTION");
  connection = N(Q(db, "SELECT CONNECTION_ID()").begin()[0]);
  LockActor(db, client);
 }

 bool Commit()
 {
  auto identity = Q(db, "SELECT CONNECTION_ID(),@@in_transaction");
  if (N(identity.begin()[0]) != connection || N(identity.begin()[1]) != 1) {
   throw std::runtime_error("Fall transaction lost its connection");
  }
  return db.QueryDatabase(std::string("COMMIT"), false).Success();
 }
};

bool FallNpc(Client& c, NPC& npc, uint32_t zone_id, uint16_t version,
             uint32_t npc_type, uint32_t spawn_id)
{
 return zone && zone->GetZoneID() == zone_id && zone->GetInstanceVersion() == version &&
  npc.GetNPCTypeID() == npc_type && npc.GetSpawnPointID() == spawn_id &&
  entity_list.GetNPCByID(npc.GetID()) == &npc && c.GetTarget() == &npc &&
  c.CalculateDistance(npc.GetX(), npc.GetY(), npc.GetZ()) <= 40.0f;
}

std::optional<FallLocation> ReadFallRun(Database& db, uint64_t run, uint32_t character, bool lock)
{
 auto rows = Q(db, "SELECT i.id,d.zone_in_x,d.zone_in_y,d.zone_in_z,d.zone_in_heading,d.has_zone_in"
  " FROM shared_tasks t JOIN shared_task_members m ON m.shared_task_id=t.id"
  " JOIN shared_task_dynamic_zones l ON l.shared_task_id=t.id"
  " JOIN dynamic_zones d ON d.id=l.dynamic_zone_id"
  " JOIN instance_list i ON i.id=d.instance_id"
  " JOIN instance_list_player p ON p.id=i.id AND p.charid=m.character_id"
  " WHERE t.id=" + S(run) + " AND t.task_id=" + S(FallTask) +
  " AND t.completion_time IS NULL AND (t.expire_time IS NULL OR t.expire_time>NOW())"
  " AND m.character_id=" + S(character) + " AND i.zone=" + S(FallZone) +
  " AND i.version=" + S(FallVersion) + " AND (i.never_expires=1 OR i.expire_at>UNIX_TIMESTAMP())" +
  (lock ? " FOR UPDATE" : ""));
 if (rows.RowCount() != 1 || !N(rows.begin()[5])) return {};
 const auto id = N(rows.begin()[0]);
 if (!id || id > UINT16_MAX) return {};
 return FallPosition(FallZone, uint32_t(id), rows.begin(), 1);
}

bool IsFallInstance(const FallLocation& location)
{
 return zone && zone->GetZoneID() == FallZone && zone->GetInstanceVersion() == FallVersion &&
  zone->GetInstanceID() == location.instance;
}

uint64_t FallRunForSession(Database& db, uint32_t character, uint64_t session)
{
 auto rows = Q(db, "SELECT run_id FROM fall_shroud_members WHERE session_id=" + S(session) +
  " AND character_id=" + S(character) + " AND exited_at IS NULL");
 if (rows.RowCount() != 1) throw std::runtime_error("Fall active member missing");
 return N(rows.begin()[0]);
}

void RequireFallSession(Database& db, uint32_t character, uint64_t session, uint64_t revision)
{
 auto rows = Q(db, "SELECT s.id FROM spirit_shroud_state a JOIN spirit_shroud_sessions s"
  " ON s.id=a.active_session_id AND s.character_id=a.character_id AND s.template_id=a.template_id"
  " WHERE a.character_id=" + S(character) + " AND s.id=" + S(session) +
  " AND a.revision=" + S(revision) + " AND s.entry_revision=a.revision AND s.phase='active'"
  " AND s.inventory_policy=" + S(FallInventoryPolicy) + " FOR UPDATE");
 if (rows.RowCount() != 1) throw std::runtime_error("Fall active session changed");
}

// This observes World-persisted activity 4. It never writes task progress or
// completed records. The durable hail intent makes a crash safe before send.
bool FinalizeFallExit(Database& db, uint64_t run, uint32_t instance)
{
 auto rows = Q(db, "SELECT exit_requested_at,succeeded_at FROM fall_shroud_runs WHERE run_id=" +
  S(run) + " AND instance_id=" + S(instance) + " FOR UPDATE");
 if (rows.RowCount() != 1 || !rows.begin()[0]) return false;
 if (rows.begin()[1]) return true;
 auto completed = Q(db, "SELECT s.shared_task_id FROM shared_task_activity_state s"
  " JOIN shared_tasks t ON t.id=s.shared_task_id WHERE t.task_id=" + S(FallTask) +
  " AND s.shared_task_id=" + S(run) +
  " AND s.activity_id=4 AND s.done_count>=1 AND s.completed_time IS NOT NULL FOR UPDATE");
 if (completed.RowCount() != 1) return false;
 auto marked = Q(db, "UPDATE fall_shroud_runs SET succeeded_at=UNIX_TIMESTAMP() WHERE run_id=" +
  S(run) + " AND instance_id=" + S(instance) + " AND succeeded_at IS NULL");
 if (marked.RowsAffected() != 1) throw std::runtime_error("Fall exit confirmation changed");
 return true;
}

std::optional<FallLocation> RestoreFallSession(Database& db, uint32_t character, uint64_t session,
                                               uint64_t revision, bool successful_exit)
{
 RequireFallSession(db, character, session, revision);
 ValidateSnapshot(db, character, session);
 const auto actor = S(character);
 const auto member = "session_id=" + S(session) + " AND character_id=" + actor + " AND exited_at IS NULL";
 auto member_row = Q(db, "SELECT run_id,instance_id,shard_token FROM fall_shroud_members"
  " WHERE " + member + " FOR UPDATE");
 if (member_row.RowCount() != 1) throw std::runtime_error("Fall member state changed");
 if (successful_exit) {
  auto run = Q(db, "SELECT run_id FROM fall_shroud_runs WHERE run_id=" + S(N(member_row.begin()[0]))+
   " AND instance_id=" + S(N(member_row.begin()[1])) + " AND succeeded_at IS NOT NULL FOR UPDATE");
  if (run.RowCount() != 1) return {};
 }
 auto original = Q(db, "SELECT zone_id,zone_instance,x,y,z,heading FROM shroud_original_character_data"
  " WHERE id=" + actor + " FOR UPDATE");
 if (original.RowCount() != 1 || N(original.begin()[0]) != FallReturnZone || N(original.begin()[1])) {
  throw std::runtime_error("Fall original return location unavailable");
 }
 const auto destination = FallPosition(FallReturnZone, 0, original.begin(), 2);
 const char* fields = "race,class,gender,level,level2,points,exp,aa_points,aa_points_spent,aa_exp,"
  "e_percent_to_aa,e_expended_aa_spent,e_aa_effects,cur_hp,mana,endurance,str,sta,cha,dex,int,agi,wis";
 std::string assignments;
 for (const auto& field : Strings::Split(fields, ',')) {
  if (!assignments.empty()) assignments += ',';
  assignments += "c.`" + field + "`=b.`" + field + "`";
 }
 Q(db, "UPDATE character_data c JOIN shroud_original_character_data b ON c.id=b.id SET " +
  assignments + "," + FallLocationSet(destination, "c.") + " WHERE c.id=" + actor);
 for (const auto& table : Tables) RestoreShroudSnapshotTable(db, table, actor, true);
 Q(db, "UPDATE spirit_shroud_experience_state SET settled_at=" + S(Now()) +
  " WHERE session_id=" + S(session) + " AND settled_at IS NULL");
 Q(db, "UPDATE spirit_shroud_sessions SET phase='restored',restored_at=" + S(Now()) +
  " WHERE id=" + S(session) + " AND phase='active'");
 Q(db, "UPDATE spirit_shroud_state SET template_id=0,active_session_id=NULL,revision=revision+1"
  " WHERE character_id=" + actor);
 const auto settled = Q(db, "UPDATE fall_shroud_members SET exited_at=" + S(Now()) +
  ",exit_succeeded=" + S(successful_exit) + " WHERE " + member);
 if (settled.RowsAffected() != 1) throw std::runtime_error("Fall member exit was not recorded");
 return destination;
}

// Settle before a zone constructs caches. The archive is authoritative. A
// duplicate load sees xp_settled_at, and cannot replay a committed reward.
void SettleFallXP(Database& db, uint32_t character)
{
 const auto actor = S(character);
 auto pending = Q(db, "SELECT m.run_id,m.session_id,m.normal_xp_bp,m.aa_xp_bp"
  " FROM fall_shroud_members m JOIN spirit_shroud_sessions s ON s.id=m.session_id AND s.phase='restored'"
  " JOIN completed_shared_task_members cm ON cm.shared_task_id=m.run_id AND cm.character_id=m.character_id"
  " JOIN completed_shared_tasks t ON t.id=cm.shared_task_id"
  " WHERE t.completion_time IS NOT NULL AND t.task_id=" + S(FallTask) + " AND m.character_id=" + actor +
  " AND m.exit_succeeded=1 AND m.exited_at IS NOT NULL AND m.shard_token IS NOT NULL"
  " AND m.xp_settled_at IS NULL ORDER BY m.run_id FOR UPDATE");
 for (auto entry : pending) {
  auto quote = Q(db, "SELECT " + std::string(ExperienceColumns) +
   " FROM spirit_shroud_experience_state WHERE session_id=" + S(N(entry[1])) + " AND settled_at IS NOT NULL");
  auto profile = Q(db, "SELECT exp,aa_exp,aa_points,aa_points_spent,level,level2,points,exp_enabled,e_percent_to_aa"
   " FROM character_data WHERE id=" + actor + " FOR UPDATE");
  if (quote.RowCount() != 1 || profile.RowCount() != 1) throw std::runtime_error("Fall XP profile unavailable");
  auto state = ReadExperience(quote.begin());
  auto row = profile.begin();
  state.normal = N(row[0]); state.aa = N(row[1]); state.aa_points = N(row[2]); state.aa_spent = N(row[3]);
  state.level = N(row[4]); state.level2 = N(row[5]); state.skill_points = N(row[6]);
  state.percent_to_aa = state.level > 50 ? std::min<uint64_t>(N(row[8]), 100) : 0;
  SpiritShroud::ValidateExperience(state);
  const auto normal_bp = N(entry[2]), aa_bp = N(entry[3]);
  if (normal_bp > 10000 || aa_bp > 10000) throw std::runtime_error("Fall XP quote invalid");
  if (N(row[7])) {
   const uint64_t bar = state.thresholds.at(state.level + 1) - state.thresholds.at(state.level);
   const auto normal = bar * normal_bp * (100 - state.percent_to_aa) / 1000000;
   const auto aa = uint64_t(state.aa_unit) * aa_bp * state.percent_to_aa / 1000000;
   SpiritShroud::ApplyExperience(state, normal, aa, RuleI(AA, UnusedAAPointCap));
   Q(db, "UPDATE character_data SET exp=" + S(state.normal) + ",aa_exp=" + S(state.aa) +
    ",aa_points=" + S(state.aa_points) + ",level=" + S(state.level) +
    ",level2=" + S(state.level2) + ",points=" + S(state.skill_points) + " WHERE id=" + actor);
  }
  auto marked = Q(db, "UPDATE fall_shroud_members SET xp_settled_at=" + S(Now()) +
   " WHERE run_id=" + S(N(entry[0])) + " AND character_id=" + actor + " AND xp_settled_at IS NULL");
  if (marked.RowsAffected() != 1) throw std::runtime_error("Fall XP marker changed");
 }
}
}

std::vector<std::shared_ptr<const SpiritShroud::Template>> Client::FallShroudProfiles(uint64_t run)
{
 std::vector<std::shared_ptr<const Template>> profiles;
 if (ClientVersion() != EQ::versions::ClientVersion::RoF2 || !ClientDataLoaded() ||
     IsSpiritShrouded() || !HasInventorySessionReservation() || GetSharedTaskId() <= 0 || uint64_t(GetSharedTaskId()) != run ||
     !zone || zone->GetZoneID() != FallReturnZone || zone->GetInstanceID() || zone->GetInstanceVersion()) return profiles;
 try {
  // Read-only UI query. All of these gates are checked again with row locks at entry.
  auto enabled = Q(database, "SELECT id FROM fall_shroud_config WHERE id=1 AND enabled=1");
  if (!enabled.RowCount() || !ReadFallRun(database, run, CharacterID(), false)) return profiles;
  auto rows = Q(database, "SELECT r.class_id,r.template_id FROM fall_shroud_roles r JOIN spirit_shroud_templates t ON t.id=r.template_id WHERE t.enabled=0 ORDER BY r.class_id");
  for (auto row : rows) {
   auto forms = Catalog(database, true, uint32_t(N(row[1])));
   if (!FallClass(N(row[0])) || forms.size() != 1 || forms.front().class_id != N(row[0]) || forms.front().level != 70 ||
       forms.front().race != 467 || forms.front().verified ||
       !forms.front().has_native_resources) return {};
   profiles.push_back(std::make_shared<const Template>(std::move(forms.front())));
  }
  if (profiles.size() != 5) return {};
 } catch (const std::exception&) {
  return {};
 }
 return profiles;
}

bool Client::SelectFallShroudRole(uint64_t run, uint32_t template_id)
{
 const auto profiles = FallShroudProfiles(run);
 if (std::none_of(profiles.begin(), profiles.end(), [template_id](const auto& form) {
      return form->id == template_id;
     })) return false;
 try {
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  ReadFallSettings(db);
  if (!ReadFallRun(db, run, CharacterID(), true)) return false;
  auto applied = Q(db, "SELECT session_id FROM fall_shroud_members WHERE run_id=" + S(run) +
   " AND character_id=" + S(CharacterID()) + " FOR UPDATE");
  if (applied.RowCount()) return false;
  Q(db, "INSERT INTO fall_shroud_choices(run_id,character_id,template_id) VALUES(" +
   S(run) + "," + S(CharacterID()) + "," + S(template_id) + ") ON DUPLICATE KEY UPDATE template_id=VALUES(template_id)");
  return transaction.Commit();
 } catch (const std::exception& error) {
  LogError("Fall role selection refused: [{}]", error.what());
  return false;
 }
}

std::optional<uint32_t> Client::FallShroudSelectedTemplate(uint64_t run)
{
 if (!run || GetSharedTaskId() <= 0 || uint64_t(GetSharedTaskId()) != run) return {};
 if (IsFallShrouded()) {
  if (zone && HasFallOfIllsalinProfile(run, zone->GetInstanceID())) return m_spirit_shroud_template;
  return {};
 }
 const auto profiles = FallShroudProfiles(run);
 if (profiles.empty()) return {};
 try {
  auto selected = Q(database, "SELECT template_id FROM fall_shroud_choices WHERE run_id=" + S(run) +
   " AND character_id=" + S(CharacterID()));
  if (selected.RowCount() != 1) return {};
  const auto id = N(selected.begin()[0]);
  for (const auto& profile : profiles) if (profile->id == id) return profile->id;
 } catch (const std::exception&) {}
 return {};
}

bool Client::HasFallOfIllsalinProfile(uint64_t occurrence, uint32_t instance)
{
 if (!occurrence || !instance || !zone || zone->GetInstanceID() != instance ||
     !IsFallShrouded() || !HasInventorySessionReservation() || m_spirit_shroud_transition ||
     !m_spirit_shroud_definition || GetSharedTaskId() <= 0 || uint64_t(GetSharedTaskId()) != occurrence) return false;
 try {
  auto destination = ReadFallRun(database, occurrence, CharacterID(), false);
  if (!destination || !IsFallInstance(*destination)) return false;
  auto state = Q(database, "SELECT s.id FROM spirit_shroud_state a JOIN spirit_shroud_sessions s"
   " ON s.id=a.active_session_id AND s.character_id=a.character_id AND s.template_id=a.template_id"
   " JOIN fall_shroud_members m ON m.session_id=s.id AND m.character_id=s.character_id"
   " WHERE a.character_id=" + S(CharacterID()) + " AND a.revision=" + S(m_spirit_shroud_revision) +
   " AND s.entry_revision=a.revision AND s.phase='active' AND s.inventory_policy=" + S(FallInventoryPolicy) +
   " AND s.id=" + S(m_spirit_shroud_session_id) + " AND s.template_id=" + S(m_spirit_shroud_template) +
   " AND s.content_version=" + S(m_spirit_shroud_definition->content_version) +
   " AND m.run_id=" + S(occurrence) + " AND m.instance_id=" + S(instance) + " AND m.exited_at IS NULL");
  return state.RowCount() == 1;
 } catch (const std::exception&) {
  return false;
 }
}

bool Client::EnterFallOfIllsalin(NPC& stillpool)
{
 const auto run = GetSharedTaskId() > 0 ? uint64_t(GetSharedTaskId()) : 0;
 if (!run || GetSharedTaskId() <= 0 || uint64_t(GetSharedTaskId()) != run ||
     IsSpiritShrouded() || m_spirit_shroud_transition || GetRaid() ||
     !FallNpc(*this, stillpool, FallReturnZone, 0, FallStillpool, FallStillpoolSpawn) ||
     zone->GetInstanceID()) return false;
 const auto profiles = FallShroudProfiles(run);
 if (profiles.empty()) return false;
 try {
  auto selected = Q(database, "SELECT template_id FROM fall_shroud_choices WHERE run_id=" + S(run) +
   " AND character_id=" + S(CharacterID()));
  if (selected.RowCount() != 1) return false;
  const auto id = N(selected.begin()[0]);
  if (!id || id > INT32_MAX) return false;
  bool queued = false;
  ChangeSpiritShroudImpl(uint32_t(id), run, &queued);
  return queued;
 } catch (const std::exception& error) {
  LogError("Fall entry refused: [{}]", error.what());
  return false;
 }
}

bool Client::RegisterFallOfIllsalinCommanderCorpse(uint64_t run, uint32_t instance, Corpse& corpse)
{
 if (!HasFallOfIllsalinProfile(run, instance) || corpse.GetEntityVariable("dodh_fall_commander").empty() ||
     !IsFallShrouded() || m_spirit_shroud_transition || !corpse.IsNPCCorpse() ||
     entity_list.GetCorpseByID(corpse.GetID()) != &corpse) return false;
 try {
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  RequireFallSession(db, CharacterID(), m_spirit_shroud_session_id, m_spirit_shroud_revision);
  if (FallRunForSession(db, CharacterID(), m_spirit_shroud_session_id) != run) return false;
  auto destination = ReadFallRun(db, run, CharacterID(), true);
  if (!destination || !IsFallInstance(*destination)) return false;
  auto member = Q(db, "SELECT commander_type FROM fall_shroud_members WHERE session_id=" + S(m_spirit_shroud_session_id));
  if (member.RowCount() != 1 || N(member.begin()[0]) != corpse.GetNPCTypeID()) return false;
  auto existing = Q(db, "SELECT commander_token FROM fall_shroud_runs WHERE run_id=" + S(run) +
   " AND instance_id=" + S(destination->instance) + " FOR UPDATE");
  if (existing.RowCount()) {
   const auto token = corpse.GetEntityVariable(FallCorpseToken);
   return existing.begin()[0] && token == existing.begin()[0];
  }
  if (!corpse.PrepareFallShardDrops()) return false;
  auto new_token = Q(db, "SELECT UUID()");
  if (new_token.RowCount() != 1 || !new_token.begin()[0]) return false;
  const std::string token = new_token.begin()[0];
  Q(db, "INSERT INTO fall_shroud_runs(run_id,instance_id,commander_token) VALUES(" +
   S(run) + "," + S(destination->instance) + ",'" + db.Escape(token) + "')");
  if (!transaction.Commit()) return false;
  corpse.SetEntityVariable(FallCorpseToken, token);
  // Use the populated-corpse decay duration now that the corpse holds shards.
  corpse.ResetDecayTimer();
  return true;
 } catch (const std::exception& error) {
  LogError("Fall commander registration refused: [{}]", error.what());
  return false;
 }
}

bool Client::TakeFallShroudShard(Corpse& corpse, uint32_t item_id, uint16_t drop_index)
{
 if (!IsFallShrouded() || m_spirit_shroud_transition || item_id != FallShard ||
     !corpse.IsNPCCorpse() || !drop_index || drop_index > 6 ||
     entity_list.GetCorpseByID(corpse.GetID()) != &corpse) return false;
 const auto token = corpse.GetEntityVariable(FallCorpseToken);
 if (token.size() != 36) return false;
 try {
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  RequireFallSession(db, CharacterID(), m_spirit_shroud_session_id, m_spirit_shroud_revision);
  const auto run = FallRunForSession(db, CharacterID(), m_spirit_shroud_session_id);
  auto destination = ReadFallRun(db, run, CharacterID(), true);
  if (!destination || !IsFallInstance(*destination)) return false;
  auto receipt = Q(db, "SELECT r.run_id FROM fall_shroud_runs r JOIN fall_shroud_members m ON m.run_id=r.run_id"
   " WHERE r.run_id=" + S(run) + " AND r.instance_id=" + S(destination->instance) +
   " AND r.commander_token='" + db.Escape(token) + "' AND r.exit_requested_at IS NULL AND r.succeeded_at IS NULL"
   " AND m.session_id=" + S(m_spirit_shroud_session_id) + " AND m.commander_type=" + S(corpse.GetNPCTypeID()) +
   " AND m.shard_token IS NULL AND m.exited_at IS NULL FOR UPDATE");
  if (receipt.RowCount() != 1) return false;
  const auto updated = Q(db, "UPDATE fall_shroud_members SET shard_token='" + db.Escape(token) +
   "',shard_slot=" + S(drop_index) + " WHERE session_id=" + S(m_spirit_shroud_session_id) +
   " AND shard_token IS NULL");
  if (updated.RowsAffected() != 1) return false;
  return transaction.Commit();
 } catch (const std::exception& error) {
  LogError("Fall shard custody refused: [{}]", error.what());
  return false;
 }
}

bool Client::ExitFallOfIllsalin(uint64_t run, uint32_t instance, NPC& draygun)
{
 if (!HasFallOfIllsalinProfile(run, instance) ||
     !FallNpc(*this, draygun, FallZone, FallVersion, FallDraygun, FallDraygunSpawn)) return false;
 if (!IsTaskActivityActive(FallTask, 4) && !IsTaskActivityActive(FallTask, 5)) return false;
 try {
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  RequireFallSession(db, CharacterID(), m_spirit_shroud_session_id, m_spirit_shroud_revision);
  auto destination = ReadFallRun(db, run, CharacterID(), true);
  if (!destination || !IsFallInstance(*destination)) return false;
  auto rows = Q(db, "SELECT run_id FROM fall_shroud_runs WHERE run_id=" + S(run) +
   " AND instance_id=" + S(destination->instance) + " FOR UPDATE");
  if (rows.RowCount() != 1) return false;
  Q(db, "UPDATE fall_shroud_runs SET exit_requested_at=COALESCE(exit_requested_at,UNIX_TIMESTAMP())"
   " WHERE run_id=" + S(run) + " AND instance_id=" + S(destination->instance));
  if (!transaction.Commit()) return false;
  if (IsTaskActivityActive(FallTask, 4)) UpdateTaskActivity(FallTask, 4, 1, true);
  m_fall_shroud_next_check = 0;
  return true; // Durable exit request accepted. Restore waits for World's activity 4.
 } catch (const std::exception& error) {
  LogError("Fall exit request refused: [{}]", error.what());
  return false;
 }
}

bool Client::RestoreFallShroud(uint64_t run, bool success)
{
 if (!IsFallShrouded() || m_spirit_shroud_transition) return false;
 try {
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  if (FallRunForSession(db, CharacterID(), m_spirit_shroud_session_id) != run) return false;
  auto destination = RestoreFallSession(db, CharacterID(), m_spirit_shroud_session_id, m_spirit_shroud_revision, success);
  if (!destination) return false;
  m_spirit_shroud_transition = true;
  if (!transaction.Commit()) {
   Disconnect();
   return false;
  }
  MovePC(destination->zone, destination->instance, destination->x, destination->y, destination->z,
   destination->heading, 0, zone->GetZoneID() == destination->zone ? EvacToSafeCoords : ZoneSolicited);
  return true;
 } catch (const std::exception& error) {
  LogError("Fall restore failed: [{}]", error.what());
  m_spirit_shroud_transition = true;
  Disconnect();
  return false;
 }
}

bool Client::PrepareFallShroudLogin()
{
 // Use the existing session table as the discriminator. Normal profiles and
 // legacy servers without the Fall tables never open a Fall transaction.
 auto candidates = database.QueryDatabase("SELECT s.id,a.revision"
  " FROM spirit_shroud_sessions s LEFT JOIN spirit_shroud_state a ON a.active_session_id=s.id"
  " AND a.character_id=s.character_id AND a.template_id=s.template_id WHERE s.inventory_policy=" +
  S(FallInventoryPolicy) + " AND s.character_id=" + S(CharacterID()) +
  " ORDER BY (a.revision IS NOT NULL) DESC,s.id DESC LIMIT 1", false);
 if (!candidates.Success()) return candidates.ErrorNumber() == 1146;
 if (!candidates.RowCount()) return true;
 try {
  if (!HasInventorySessionReservation()) throw std::runtime_error("Fall login lease unavailable");
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  auto active = Q(db, "SELECT s.id,a.revision FROM spirit_shroud_state a"
   " JOIN spirit_shroud_sessions s ON s.id=a.active_session_id WHERE a.character_id=" +
   S(CharacterID()) + " AND s.inventory_policy=" + S(FallInventoryPolicy) + " FOR UPDATE");
  if (active.RowCount()) {
   const auto session = N(active.begin()[0]), revision = N(active.begin()[1]);
   RequireFallSession(db, CharacterID(), session, revision);
   const auto run = FallRunForSession(db, CharacterID(), session);
   auto member = Q(db, "SELECT instance_id FROM fall_shroud_members WHERE session_id=" + S(session) + " FOR UPDATE");
   if (member.RowCount() != 1) throw std::runtime_error("Fall login instance unbound");
   const bool success = FinalizeFallExit(db, run, uint32_t(N(member.begin()[0])));
   auto destination = ReadFallRun(db, run, CharacterID(), true);
   if (!success && destination && IsFallInstance(*destination) &&
       ClientVersion() == EQ::versions::ClientVersion::RoF2) return true;
   auto restored = RestoreFallSession(db, CharacterID(), session, revision, success);
   if (!restored) return false;
   // Do not leave a normal profile loaded into the old instance. The next
   // world login routes to the restored position from committed storage.
   if (!transaction.Commit()) return false;
   return zone->GetZoneID() == restored->zone && zone->GetInstanceID() == restored->instance;
  }
  auto normal = Q(db, "SELECT template_id FROM spirit_shroud_state WHERE character_id=" +
   S(CharacterID()) + " FOR UPDATE");
  if (normal.RowCount() && N(normal.begin()[0])) return true;
  SettleFallXP(db, CharacterID());
  return transaction.Commit();
 } catch (const std::exception& error) {
  LogError("Fall admission/recovery refused: [{}]", error.what());
  return false;
 }
}

void Client::RequestFallShroudXpRefresh(uint64_t run)
{
 if (!run || IsSpiritShrouded() || m_spirit_shroud_transition || !zone ||
     zone->GetZoneID() != FallReturnZone || zone->GetInstanceID() || zone->GetInstanceVersion()) return;
 m_fall_shroud_refresh_run = run;
 m_fall_shroud_refresh_deadline = Now() + 20;
 m_fall_shroud_next_check = Now() + 1;
}

bool Client::ProcessFallShroud()
{
 if (m_fall_shroud_refresh_run && !IsSpiritShrouded() && !m_spirit_shroud_transition &&
     ClientDataLoaded() && Now() >= m_fall_shroud_next_check) {
  m_fall_shroud_next_check = Now() + 1;
  if (Now() > m_fall_shroud_refresh_deadline) {
   m_fall_shroud_refresh_run = 0; // The next zone load still reconciles the durable archive.
   return true;
  }
  try {
   auto ready = Q(database, "SELECT t.id FROM completed_shared_tasks t"
    " JOIN completed_shared_task_members c ON c.shared_task_id=t.id"
    " JOIN fall_shroud_members m ON m.run_id=t.id AND m.character_id=c.character_id"
    " WHERE t.id=" + S(m_fall_shroud_refresh_run) + " AND t.task_id=" + S(FallTask) +
    " AND t.completion_time IS NOT NULL AND c.character_id=" + S(CharacterID()) +
    " AND m.exit_succeeded=1 AND m.exited_at IS NOT NULL AND m.shard_token IS NOT NULL"
    " AND m.xp_settled_at IS NULL");
   if (ready.RowCount() && !IsEngaged() && !IsCasting() && !IsLooting() &&
       (!trade || trade->state==TradeNone) && HasInventorySessionReservation() && SaveSpiritShroudCheckpoint()) {
    m_fall_shroud_refresh_run = 0;
    m_spirit_shroud_transition = true;
    MovePC(FallReturnZone, 0, GetX(), GetY(), GetZ(), GetHeading(), 0, EvacToSafeCoords);
   }
  } catch (const std::exception& error) {
   m_fall_shroud_refresh_run = 0;
   LogError("Fall XP refresh deferred to next login: [{}]", error.what());
  }
  return true;
 }
 if (!IsFallShrouded() || m_spirit_shroud_transition || !ClientDataLoaded() ||
     Now() < m_fall_shroud_next_check) return true;
 m_fall_shroud_next_check = Now() + 5;
 try {
  const auto run = FallRunForSession(database, CharacterID(), m_spirit_shroud_session_id);
  auto exit_request = Q(database, "SELECT r.instance_id FROM fall_shroud_runs r JOIN fall_shroud_members m"
   " ON m.run_id=r.run_id AND m.instance_id=r.instance_id WHERE m.session_id=" + S(m_spirit_shroud_session_id) +
   " AND r.exit_requested_at IS NOT NULL");
  if (exit_request.RowCount()) {
   bool succeeded = false;
   {
    FallTransaction transaction(*this);
    RequireFallSession(transaction.db, CharacterID(), m_spirit_shroud_session_id, m_spirit_shroud_revision);
    succeeded = FinalizeFallExit(transaction.db, run, uint32_t(N(exit_request.begin()[0])));
    if (succeeded && !transaction.Commit()) throw std::runtime_error("Fall successful exit commit uncertain");
   }
   if (succeeded) return RestoreFallShroud(run, true);
  }
  auto location = ReadFallRun(database, run, CharacterID(), false);
  if (location && IsFallInstance(*location)) {
   if (exit_request.RowCount() && IsTaskActivityActive(FallTask, 4)) UpdateTaskActivity(FallTask, 4, 1, true);
   return true;
  }
  return RestoreFallShroud(run, false);
 } catch (const std::exception& error) {
  LogError("Fall run monitor failed: [{}]", error.what());
  m_spirit_shroud_transition = true;
  Disconnect();
  return false;
 }
}

bool Client::TryFallShroudDeath()
{
 if (!IsFallShrouded()) return false;
 if (m_spirit_shroud_transition) return true;
 try {
  FallTransaction transaction(*this);
  auto& db = transaction.db;
  RequireFallSession(db, CharacterID(), m_spirit_shroud_session_id, m_spirit_shroud_revision);
  const auto run = FallRunForSession(db, CharacterID(), m_spirit_shroud_session_id);
  auto location = ReadFallRun(db, run, CharacterID(), true);
  if (!location || !IsFallInstance(*location)) {
   // Release transaction locks before the next ZoneEntry recovers.
   m_spirit_shroud_transition = true;
   Disconnect();
   return true;
  }
  auto rows = Q(db, "SELECT respawn_x,respawn_y,respawn_z,respawn_heading,refill_percent"
   " FROM fall_shroud_members WHERE session_id=" + S(m_spirit_shroud_session_id) + " FOR UPDATE");
  if (rows.RowCount() != 1) throw std::runtime_error("Fall respawn data missing");
  auto target = FallPosition(FallZone, location->instance, rows.begin(), 0);
  const auto percent = N(rows.begin()[4]);
  if (!percent || percent > 100) throw std::runtime_error("Fall refill percent invalid");
  auto refill = [percent](int64_t maximum) {
   return std::max<int64_t>(0, maximum) * int64_t(percent) / 100;
  };
  const auto hp = std::max<int64_t>(1, refill(GetMaxHP()));
  const auto mana = refill(GetMaxMana()), endurance = refill(GetMaxEndurance());
  Q(db, "DELETE FROM character_buffs WHERE character_id=" + S(CharacterID()));
  Q(db, "UPDATE character_data SET " + FallLocationSet(target) + ",cur_hp=" + S(hp) +
   ",mana=" + S(mana) + ",endurance=" + S(endurance) + " WHERE id=" + S(CharacterID()));
  if (!transaction.Commit()) throw std::runtime_error("Fall respawn commit uncertain");
  InterruptSpell();
  entity_list.RemoveFromTargets(this, true);
  hate_list.RemoveEntFromHateList(this);
  RemoveAutoXTargets();
  ClearAllProximities();
  SetTarget(nullptr);
  BuffFadeAll();
  CalcBonuses();
  SetHP(std::min<int64_t>(hp, GetMaxHP()));
  SetMana(std::min<int64_t>(mana, GetMaxMana()));
  SetEndurance(std::min<int64_t>(endurance, GetMaxEndurance()));
  GMMove(target.x, target.y, target.z, target.heading);
  SendHPUpdate(true);
 } catch (const std::exception& error) {
  LogError("Fall local respawn failed: [{}]", error.what());
  m_spirit_shroud_transition = true;
  Disconnect();
 }
 return true;
}
