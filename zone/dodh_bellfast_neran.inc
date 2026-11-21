#include "map.h"
#include "water_map.h"

#include <chrono>
#include <unordered_map>

namespace DodhBellfastNeran {
namespace Travel = DodhLongshadowTravel;
using Clock = std::chrono::steady_clock;

constexpr uint32_t Task = 920273;
constexpr uint32_t Cocoon = 347030;
constexpr uint32_t CocoonSpawn = 102910;
constexpr uint32_t Actor = 347940;
constexpr uint16_t Force = 8403;
constexpr const char* Marker = "rof2_dodh_neran_scope";
constexpr const char* Leg = "rof2_dodh_neran_leg";
const glm::vec4 CocoonPoint{-771.0f, -160.0f, 87.375f, 250.0f};
const glm::vec4 Center{-538.51f, -923.76f, 108.18f, 0.0f};
const glm::vec4 Departure{-493.46f, -1361.10f, 103.91f, 0.0f};
const glm::vec4 Landing{-494.53f, -1533.37f, 101.24f, 0.0f};
constexpr float FarShoreNorthY = -1518.0f;
constexpr float FarShoreSouthY = -1590.0f;

bool PublicIllsalin()
{
    return zone && zone->GetZoneID() == 347 && zone->GetInstanceVersion() == 0 &&
        !zone->GetInstanceID();
}

struct Run {
    uint64_t id;
    uint32_t accepted;
    const DynamicZone* dz;

    std::string Stamp() const { return fmt::format("{}:{}", id, accepted); }
};

std::optional<Run> Member(Client& client)
{
    if (!PublicIllsalin() || client.IsSpiritShrouded() ||
        client.GetHP() <= 0) return std::nullopt;
    const auto* mission = Travel::ActiveMission(client);
    if (!mission || mission->task != Task) return std::nullopt;
    const auto* dz = Travel::OwnedMission(client, *mission, false);
    const auto accepted = client.GetTaskState()->GetActiveSharedTask().accepted_time;
    if (!dz || accepted <= 0 || uint64_t(accepted) > UINT32_MAX) return std::nullopt;
    return Run{uint64_t(client.GetSharedTaskId()), uint32_t(accepted), dz};
}

std::string Scope(Client& client, const Run& run)
{
    return fmt::format(
        "live.id={} AND live.task_id=920273 AND m.character_id={} AND dz.id={} AND i.id={} "
        "AND i.zone=349 AND i.version=2 AND UNIX_TIMESTAMP(live.accepted_time)={} "
        "AND live.completion_time IS NULL AND (live.expire_time IS NULL OR live.expire_time>NOW()) "
        "AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP()",
        run.id, client.CharacterID(), run.dz->GetID(), run.dz->GetInstanceID(), run.accepted);
}

constexpr const char* SceneJoin =
    " JOIN dodh_bellfast_neran_runs scene ON scene.shared_task_id=live.id "
    "AND scene.accepted_time=UNIX_TIMESTAMP(live.accepted_time) AND scene.instance_id=i.id ";
constexpr const char* CrossingJoin =
    " JOIN dodh_bellfast_neran_crossings crossing ON crossing.shared_task_id=scene.shared_task_id "
    "AND crossing.accepted_time=scene.accepted_time AND crossing.instance_id=scene.instance_id "
    "AND crossing.character_id=m.character_id AND crossing.spell_id=8403 ";

std::optional<unsigned> Phase(Client& client, const Run& run)
{
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT scene.phase FROM shared_tasks live{}{} WHERE {}",
        Travel::ScopeJoin, SceneJoin, Scope(client, run)), false);
    if (!rows.Success() || rows.RowCount() != 1) return std::nullopt;
    const auto phase = SharedTaskAdmission::Number(rows.begin()[0]);
    return phase && *phase <= 2 ? std::optional<unsigned>(*phase) : std::nullopt;
}

bool Advance(Client& client, const Run& run, unsigned phase)
{
    const auto result = database.QueryDatabase(fmt::format(
        "UPDATE shared_tasks live{}{} SET scene.phase={} WHERE {} AND scene.phase={}",
        Travel::ScopeJoin, SceneJoin, phase, Scope(client, run), phase - 1), false);
    return result.Success() && (result.RowsAffected() == 1 || Phase(client, run) == phase);
}

bool Near(const glm::vec4& observed, const glm::vec4& expected, float radius, float vertical)
{
    return std::isfinite(observed.x) && std::isfinite(observed.y) && std::isfinite(observed.z) &&
        std::abs(observed.z - expected.z) <= vertical &&
        DistanceSquaredNoZ(observed, expected) <= radius * radius;
}

bool MatchActor(NPC& npc, const Run& run)
{
    return npc.GetNPCTypeID() == Actor && !npc.GetSpawnPointID() && !npc.GetDepop() &&
        npc.GetHP() > 0 && npc.GetEntityVariable(Marker) == run.Stamp();
}

NPC* FindActor(const Run& run)
{
    for (const auto& entry : entity_list.GetNPCList()) {
        if (MatchActor(*entry.second, run)) return entry.second;
    }
    return nullptr;
}

NPC* ShowActor(const Run& run, unsigned phase)
{
    if (auto* present = FindActor(run)) return present;
    const auto* type = content_db.LoadNPCTypesData(Actor);
    if (!type) return nullptr;
    auto* npc = new NPC(type, nullptr, phase == 2 ? Departure : CocoonPoint, GravityBehavior::Water);
    npc->SetEntityVariable(Marker, run.Stamp());
    npc->SetEntityVariable(Leg, phase == 2 ? "ready" : "cocoon");
    entity_list.AddNPC(npc);
    return npc;
}

bool RoomClear()
{
    // Retained hostile spawns in and immediately below the cocoon room.
    // The adjacent docile placeholder is irrelevant unless the hostile rare is up.
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetDepop() || npc->GetHP() <= 0) continue;
        switch (npc->GetSpawnPointID()) {
        case 102884: case 102890: case 102900: case 102905: case 102906: case 102907:
            return false;
        case 102909:
            if (npc->GetNPCTypeID() == 347099) return false;
            break;
        default:
            break;
        }
    }
    return true;
}

bool Open(Client& client, NPC& target)
{
    if (!PublicIllsalin() || target.GetNPCTypeID() != Cocoon ||
        target.GetSpawnPointID() != CocoonSpawn) return false;
    const auto run = Member(client);
    if (!run || target.GetDepop() || target.GetHP() <= 0 ||
        !Near(client.GetPosition(), target.GetPosition(), 20.0f, 12.0f)) return true;
    if (!client.IsTaskActivityActive(Task, 0)) return true;
    if (!RoomClear()) {
        client.Message(Chat::Yellow, "Clear the guards around the cocoon first.");
        return true;
    }
    if (!content_db.LoadNPCTypesData(Actor)) {
        client.Message(Chat::Yellow, "Neran is unavailable right now.");
        return true;
    }
    const auto result = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_bellfast_neran_runs (shared_task_id,accepted_time,instance_id,opened_by,phase) "
        "SELECT live.id,UNIX_TIMESTAMP(live.accepted_time),i.id,m.character_id,0 FROM shared_tasks live{} "
        "JOIN shared_task_activity_state a ON a.shared_task_id=live.id AND a.activity_id=0 "
        "AND a.done_count=0 AND a.completed_time IS NULL WHERE {} "
        "ON DUPLICATE KEY UPDATE shared_task_id=dodh_bellfast_neran_runs.shared_task_id",
        Travel::ScopeJoin, Scope(client, *run)), false);
    if (!result.Success()) return true;
    const auto phase = Phase(client, *run);
    if (phase && ShowActor(*run, *phase)) Travel::Record(client, *Travel::Find(Task), 0);
    return true;
}

bool HasCrossing(Client& client, const Run& run)
{
    const auto receipt = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM shared_tasks live{}{}{} WHERE {} AND scene.phase=2",
        Travel::ScopeJoin, SceneJoin, CrossingJoin, Scope(client, run)), false);
    return receipt.Success() && receipt.RowCount() == 1;
}

bool CanEnter(Client& client)
{
    const auto run = Member(client);
    if (run && HasCrossing(client, *run)) return true;
    client.Message(Chat::Yellow, "Cross the broken bridge with Neran before entering the Temple.");
    return false;
}

struct Flight {
    uint64_t run;
    uint32_t accepted;
    uint32_t instance;
    uint16_t receiver;
    uint16_t caster;
    glm::vec4 launch;
    Clock::time_point expires;
};

std::unordered_map<uint32_t, Flight> flights;

struct ArmedCast {
    Client& client;
    NPC& npc;
    const Run& run;
    glm::vec4 launch;
};

thread_local const ArmedCast* armed = nullptr;

void ForceLanded(NPC& npc, Client& client)
{
    if (!armed || &armed->npc != &npc || &armed->client != &client ||
        !PublicIllsalin() || !MatchActor(npc, armed->run)) return;
    const auto& run = armed->run;
    flights.insert_or_assign(client.CharacterID(), Flight{
        run.id, run.accepted, run.dz->GetInstanceID(), client.GetID(), npc.GetID(),
        armed->launch, Clock::now() + std::chrono::seconds(10)});
    LogQuestDebugDetail("Neran flight registered run={} accepted={} instance={} character={} caster={} receiver={} launch=({},{},{})",
        run.id, run.accepted, run.dz->GetInstanceID(), client.CharacterID(), npc.GetID(), client.GetID(),
        armed->launch.x, armed->launch.y, armed->launch.z);
}

bool OnDryBridgeSurface(const glm::vec4& point)
{
    if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z) ||
        !zone->HasMap() || !zone->HasWaterMap()) return false;

    glm::vec3 probe(point);
    const float floor = zone->zonemap->FindBestZ(probe, nullptr);
    const float clearance = point.z - floor;
    return std::isfinite(floor) && floor >= 98.0f && floor <= 108.0f &&
        clearance >= 0.0f && clearance <= 7.0f &&
        !zone->watermap->InLiquid(glm::vec3(point.x, point.y, floor + 0.1f)) &&
        !zone->watermap->InLiquid(glm::vec3(point));
}

bool Hail(Client& client, NPC& npc)
{
    if (!PublicIllsalin() || npc.GetNPCTypeID() != Actor) return false;
    const auto run = Member(client);
    if (!run || !MatchActor(npc, *run) ||
        !Near(client.GetPosition(), npc.GetPosition(), 22.0f, 12.0f)) return true;
    const auto phase = Phase(client, *run);
    if (!phase) return true;
    if (*phase != 2) {
        if (!client.IsTaskActivityActive(Task, 1) || npc.GetGrid() != 0) return true;
        const auto leg = npc.GetEntityVariable(Leg);
        if (leg == "cocoon") {
            if (!Near(npc.GetPosition(), CocoonPoint, 18.0f, 14.0f) ||
                (*phase == 0 && !Advance(client, *run, 1))) return true;
            npc.SetEntityVariable(Leg, "center");
            npc.SetRunning(true);
            // Quest movement updates the guard destination as well as the active waypoint.
            npc.MoveTo(Center, true);
            client.Message(Chat::Yellow, "Follow Neran to the broken bridge.");
            return true;
        }
        // Zone wake restores the stamped leg and position but not the quest waypoint.
        // Keep an active order intact and let Progress witness any arrival already reached.
        if (*phase == 1 && (leg == "center" || leg == "depart") && !npc.IsMoving() &&
            npc.GetCurWp() != EQ::WaypointStatus::QuestControlNoGrid) {
            const auto& destination = leg == "center" ? Center : Departure;
            if (!Near(npc.GetPosition(), destination, 9.0f, 8.0f)) {
                npc.SetRunning(true);
                npc.MoveTo(destination, true);
                client.Message(Chat::Yellow, "Follow Neran to the broken bridge.");
            }
        }
        return true;
    }
    if (HasCrossing(client, *run)) {
        client.Message(Chat::Yellow, "Your crossing is recorded. The Temple entrance is across the bridge.");
        return true;
    }
    if (const auto pending = flights.find(client.CharacterID()); pending != flights.end() &&
        pending->second.receiver == client.GetID() && pending->second.expires > Clock::now())
        return true;
    const auto point = client.GetPosition();
    if (!Near(npc.GetPosition(), Departure, 12.0f, 10.0f) ||
        !Near(point, Departure, 18.0f, 10.0f) || point.y > npc.GetY() - 1.0f || !OnDryBridgeSurface(point)) {
        client.Message(Chat::Yellow, "Stand just past Neran toward the gap, then hail him.");
        return true;
    }
    if (!IsValidSpell(Force) || spells[Force].push_back != 12.0f || spells[Force].push_up != 6.0f ||
        armed) return true;
    npc.SetHeading(npc.CalculateHeadingToTarget(Landing.x, Landing.y));
    const ArmedCast cast{client, npc, *run, point};
    struct Reset { ~Reset() { armed = nullptr; } } reset;
    armed = &cast;
    npc.SpellFinished(Force, &client, EQ::spells::CastingSlot::Item, 0, -1, spells[Force].resist_difficulty);
    return true;
}

void ObserveMovement(Client& client, bool on_boat)
{
    if (!PublicIllsalin()) return;
    auto it = flights.find(client.CharacterID());
    if (it == flights.end()) return;
    const auto& flight = it->second;
    const bool expired = flight.expires <= Clock::now();
    if (expired || flight.receiver != client.GetID() ||
        client.GetSharedTaskId() <= 0 || uint64_t(client.GetSharedTaskId()) != flight.run ||
        client.GetHP() <= 0 || on_boat) {
        LogQuestDebugDetail("Neran flight invalidated run={} character={} expired={} receiver={} expected_receiver={} active_run={} hp={} boat={}",
            flight.run, client.CharacterID(), expired, client.GetID(), flight.receiver, client.GetSharedTaskId(),
            client.GetHP(), on_boat);
        flights.erase(it);
        return;
    }

    // RoF2 need not send every position during knockback. One real update
    // on the far platform completes the witness armed by the exact cast.
    const auto& point = client.GetPosition();
    if (point.y > FarShoreNorthY || point.y < FarShoreSouthY ||
        std::abs(point.x - Departure.x) > 36.0f || !OnDryBridgeSurface(point)) return;

    const auto run = Member(client);
    if (!run || run->id != flight.run || run->accepted != flight.accepted ||
        run->dz->GetInstanceID() != flight.instance) {
        LogQuestDebugDetail("Neran flight invalidated at dry landing run={} character={} expected_instance={} member={} member_run={} member_epoch={} member_instance={}",
            flight.run, client.CharacterID(), flight.instance, run.has_value(), run ? run->id : 0,
            run ? run->accepted : 0, run ? run->dz->GetInstanceID() : 0);
        flights.erase(it);
        return;
    }
    const auto result = database.QueryDatabase(fmt::format(
        "INSERT INTO dodh_bellfast_neran_crossings "
        "(shared_task_id,accepted_time,instance_id,character_id,caster_entity_id,spell_id,from_x,from_y,from_z,to_x,to_y,to_z) "
        "SELECT live.id,UNIX_TIMESTAMP(live.accepted_time),i.id,m.character_id,{},8403,{},{},{},{},{},{} "
        "FROM shared_tasks live{}{} WHERE {} AND scene.phase=2 "
        "ON DUPLICATE KEY UPDATE character_id=dodh_bellfast_neran_crossings.character_id",
        flight.caster, flight.launch.x, flight.launch.y, flight.launch.z,
        point.x, point.y, point.z, Travel::ScopeJoin, SceneJoin, Scope(client, *run)), false);
    const bool confirmed = result.Success() && HasCrossing(client, *run);
    LogQuestDebugDetail("Neran dry landing run={} instance={} character={} point=({},{},{}) insert_ok={} affected={} receipt={}",
        run->id, flight.instance, client.CharacterID(), point.x, point.y, point.z, result.Success(),
        result.RowsAffected(), confirmed);
    if (!confirmed) return;
    flights.erase(it);
    if (client.IsTaskActivityActive(Task, 1)) Travel::Record(client, *Travel::Find(Task), 1);
    client.Message(Chat::Yellow, "Your crossing is recorded. The Temple entrance is ahead.");
}

void Sweep()
{
    static auto next = Clock::time_point{};
    const auto now = Clock::now();
    if (now < next) return;
    next = now + std::chrono::seconds(15);
    std::erase_if(flights, [&](const auto& row) {
        const bool expired = row.second.expires <= now;
        if (expired) {
            LogQuestDebugDetail("Neran flight invalidated run={} character={} instance={} expired=true",
                row.second.run, row.first, row.second.instance);
        }
        return expired;
    });
    for (const auto& entry : entity_list.GetNPCList()) {
        auto* npc = entry.second;
        if (npc->GetNPCTypeID() != Actor || npc->GetSpawnPointID() || npc->GetDepop()) continue;
        const auto stamp = npc->GetEntityVariable(Marker);
        if (stamp.empty()) continue;
        const auto& clients = entity_list.GetClientList();
        const bool occupied = std::any_of(clients.begin(), clients.end(), [&](const auto& player) {
            auto& client = *player.second;
            const auto* mission = Travel::ActiveMission(client);
            return mission && mission->task == Task &&
                fmt::format("{}:{}", client.GetSharedTaskId(),
                    client.GetTaskState()->GetActiveSharedTask().accepted_time) == stamp;
        });
        if (!occupied) npc->Depop(false);
    }
}

void Progress(Client& client)
{
    if (!PublicIllsalin()) return;
    Sweep();
    const auto* mission = Travel::ActiveMission(client);
    if (!mission || mission->task != Task) return;
    const bool locate = client.IsTaskActivityActive(Task, 0);
    const bool crossing = client.IsTaskActivityActive(Task, 1);
    if (!locate && !crossing && !Near(client.GetPosition(), Departure, 35.0f, 15.0f)) return;
    const auto run = Member(client);
    if (!run) return;
    if (crossing && HasCrossing(client, *run)) {
        Travel::Record(client, *mission, 1);
        return;
    }
    const auto phase = Phase(client, *run);
    if (!phase || (!locate && !crossing && *phase != 2)) return;
    auto* npc = FindActor(*run);
    if (!npc && Near(client.GetPosition(), *phase == 2 ? Departure : CocoonPoint, 35.0f, 15.0f))
        npc = ShowActor(*run, *phase);
    if (!npc) return;
    if (locate && Near(client.GetPosition(), npc->GetPosition(), 22.0f, 12.0f)) {
        Travel::Record(client, *mission, 0);
        return;
    }
    const auto leg = npc->GetEntityVariable(Leg);
    if (*phase == 1 && leg == "center" && Near(npc->GetPosition(), Center, 9.0f, 8.0f)) {
        npc->SetEntityVariable(Leg, "depart");
        npc->MoveTo(Departure, true);
    } else if (*phase == 1 && leg == "depart" &&
        Near(npc->GetPosition(), Departure, 9.0f, 8.0f) && Advance(client, *run, 2)) {
        npc->StopNavigation();
        npc->SetEntityVariable(Leg, "ready");
        client.Message(Chat::Yellow, "Neran is ready at the broken bridge.");
    }
}
} // namespace DodhBellfastNeran
