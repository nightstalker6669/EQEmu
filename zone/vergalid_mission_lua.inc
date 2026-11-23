#include "vergalid_mission_lua.h"
#include "zone/npc.h"
#include "common/repositories/character_expedition_lockouts_repository.h"
#include "common/repositories/dynamic_zone_lockouts_repository.h"
#include "common/shared_task_admission.h"
#include "common/spdat.h"

#include <algorithm>
#include <ctime>

namespace VergalidMission {
namespace {
using SharedTaskAdmission::Number;

struct Binding {
    DynamicZone* dz = nullptr;
    uint64_t run = 0;
    bool active = false;
};

Binding Resolve(Client& client, uint32_t task, bool lock = false)
{
    const auto version = Version(task);
    if (!version || !zone || zone->GetZoneID() != Zone || zone->GetInstanceVersion() != version ||
        !zone->GetInstanceID() || client.IsSpiritShrouded()) return {};
    auto* dz = zone->GetDynamicZone();
    const auto who = client.CharacterID();
    if (!dz || !dz->IsCurrentZoneDz() || dz->GetType() != DynamicZoneType::Mission ||
        !dz->IsValid() || dz->IsExpired() || !dz->HasMember(who) || dz->GetUUID().size() != 36 ||
        (task == End && dz->GetName() != Name)) return {};

    auto rows = database.QueryDatabase(fmt::format(
        "SELECT s.id,s.completion_time IS NULL FROM shared_tasks s "
        "JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=s.id "
        "JOIN dynamic_zones d ON d.id=sd.dynamic_zone_id "
        "JOIN instance_list i ON i.id=d.instance_id "
        "JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=d.id AND dm.character_id={0} "
        "JOIN shared_task_members sm ON sm.shared_task_id=s.id AND sm.character_id={0} "
        "WHERE d.id={1} AND d.type={2} AND d.uuid='{3}' AND i.id={4} AND i.zone=404 AND i.version={5} "
        "AND s.task_id={6} AND (s.expire_time IS NULL OR s.expire_time>NOW()) AND "
        "(s.completion_time IS NULL OR EXISTS (SELECT 1 FROM completed_shared_tasks a "
        "JOIN completed_shared_task_members am ON am.shared_task_id=a.id AND am.character_id={0} "
        "WHERE a.id=s.id AND a.task_id=s.task_id AND a.completion_time IS NOT NULL)){7}",
        who, dz->GetID(), static_cast<int>(DynamicZoneType::Mission),
        database.Escape(dz->GetUUID()), dz->GetInstanceID(), version, task, lock ? " FOR UPDATE" : ""));
    if (!rows.Success() || rows.RowCount() != 1) return {};
    auto& row = rows.begin();
    const auto run = Number(row[0]);
    const auto active = Number(row[1]);
    if (!run || !*run || !active || *active > 1 ||
        (*active && (client.GetSharedTaskId() <= 0 ||
            static_cast<uint64_t>(client.GetSharedTaskId()) != *run))) return {};
    return {dz, *run, *active != 0};
}

std::string Status(const DynamicZone& dz, const Event& event, bool lock = false)
{
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT from_expedition_uuid FROM dynamic_zone_lockouts WHERE dynamic_zone_id={} "
        "AND BINARY event_name='{}' AND expire_time>NOW(){}",
        dz.GetID(), database.Escape(std::string(event.name)), lock ? " FOR UPDATE" : ""));
    if (!rows.Success() || rows.RowCount() > 1) return {};
    if (!rows.RowCount()) return "none";
    const auto* uuid = rows.begin()[0];
    if (!uuid || std::string_view(uuid).size() != 36) return {};
    return uuid == dz.GetUUID() ? "earned" : "inherited";
}

// Must be called inside the same serializable write path as event publication.
std::vector<uint32_t> LockRoster(const Binding& scope)
{
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT dm.character_id,sm.character_id FROM dynamic_zone_members dm "
        "LEFT JOIN shared_task_members sm ON sm.shared_task_id={} AND sm.character_id=dm.character_id "
        "WHERE dm.dynamic_zone_id={} ORDER BY dm.character_id FOR UPDATE", scope.run, scope.dz->GetID()));
    if (!rows.Success()) return {};
    std::vector<uint32_t> members;
    for (auto row = rows.begin(); row != rows.end(); ++row) {
        const auto id = Number(row[0]);
        if (!id || !*id || *id > UINT32_MAX || Number(row[1]) != id) return {};
        members.push_back(static_cast<uint32_t>(*id));
    }
    auto cached = scope.dz->GetMemberIds();
    std::sort(cached.begin(), cached.end());
    if (members != cached || members.empty()) return {};
    auto count = database.QueryDatabase(fmt::format(
        "SELECT COUNT(*) FROM shared_task_members WHERE shared_task_id={} FOR UPDATE", scope.run));
    if (!count.Success() || count.RowCount() != 1 || Number(count.begin()[0]) != members.size()) return {};
    for (const auto& entry : entity_list.GetClientList()) {
        if (!entry.second || !std::binary_search(members.begin(), members.end(), entry.second->CharacterID())) return {};
    }
    return members;
}
}

std::string UUID(Client& client, uint32_t task)
{
    const auto scope = Resolve(client, task);
    return scope.dz ? scope.dz->GetUUID() : std::string{};
}

std::string EventStatus(Client& client, const std::string& event_name)
{
    const auto* event = FindEvent(event_name);
    const auto scope = event ? Resolve(client, End) : Binding{};
    return scope.dz ? Status(*scope.dz, *event) : std::string{};
}

bool AwardEvent(Client& client, const std::string& event_name)
{
    const auto* event = FindEvent(event_name);
    if (!event || !Resolve(client, End).dz || !database.TransactionBeginStrict().Success()) return false;
    struct Rollback { ~Rollback() { database.TransactionRollbackStrict(); } } rollback;
    const auto scope = Resolve(client, End, true);
    if (!scope.dz) return false;

    auto engines = database.QueryDatabase(
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' "
        "AND table_name IN ('dynamic_zone_lockouts','character_expedition_lockouts','dynamic_zone_members','shared_task_members')");
    if (!engines.Success() || engines.RowCount() != 1 || Number(engines.begin()[0]) != 4) return false;
    const auto members = LockRoster(scope);
    if (members.empty()) return false;
    const auto status = Status(*scope.dz, *event, true);
    if (status != "none" && status != "earned") return false;

    const auto predicate = fmt::format(
        "character_id IN ({}) AND BINARY expedition_name='{}' AND BINARY event_name='{}' AND expire_time>NOW()",
        fmt::join(members, ","), database.Escape(Name), database.Escape(event_name));
    const auto uuid = database.Escape(scope.dz->GetUUID());
    auto conflicts = database.QueryDatabase(fmt::format(
        "SELECT character_id FROM character_expedition_lockouts WHERE {} AND from_expedition_uuid<>'{}' FOR UPDATE",
        predicate, uuid));
    if (!conflicts.Success() || conflicts.RowCount()) return false;
    const auto readback = [&] {
        auto rows = database.QueryDatabase(fmt::format(
            "SELECT character_id FROM character_expedition_lockouts WHERE {} AND from_expedition_uuid='{}'",
            predicate, uuid));
        return rows.Success() && rows.RowCount() == members.size();
    };
    if (status == "earned") {
        if (!readback()) return false;
        auto persisted = database.QueryDatabase(fmt::format(
            "SELECT UNIX_TIMESTAMP(expire_time),duration FROM dynamic_zone_lockouts "
            "WHERE dynamic_zone_id={} AND BINARY event_name='{}' AND from_expedition_uuid='{}' AND expire_time>NOW()",
            scope.dz->GetID(), database.Escape(event_name), uuid));
        if (!persisted.Success() || persisted.RowCount() != 1) return false;
        auto& row = persisted.begin();
        const auto expiry = Number(row[0]);
        const auto duration = Number(row[1]);
        if (!expiry || !*expiry || !duration || *duration != event->seconds ||
            !database.TransactionCommitStrict().Success()) return false;
        // Rehydrate the same known result after an uncertain prior publish.
        if (!std::ranges::any_of(scope.dz->GetLockouts(), [&](const auto& lockout) {
                return lockout.IsEvent(event_name) && lockout.UUID() == scope.dz->GetUUID() &&
                    !lockout.IsExpired();
            })) {
            scope.dz->PublishVergalidLockout(DzLockout{scope.dz->GetUUID(), Name, event_name,
                static_cast<time_t>(*expiry), static_cast<uint32_t>(*duration)});
        }
        return true;
    }

    DzLockout lockout{scope.dz->GetUUID(), Name, event_name,
        std::time(nullptr) + event->seconds, event->seconds};
    DynamicZoneLockoutsRepository::InsertLockouts(database, scope.dz->GetID(), {lockout});
    CharacterExpeditionLockoutsRepository::InsertLockout(database, members, lockout);
    if (database.TransactionStrictFailed() || !readback() || Status(*scope.dz, *event) != "earned" ||
        !database.TransactionCommitStrict().Success()) return false;
    scope.dz->PublishVergalidLockout(lockout);
    return true;
}

bool EventLoot(Client& client, const std::string& event_name)
{
    const auto* event = FindEvent(event_name);
    const auto scope = event ? Resolve(client, End) : Binding{};
    if (!scope.dz || Status(*scope.dz, *event) != "earned") return false;
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT character_id FROM character_expedition_lockouts WHERE character_id={} "
        "AND BINARY expedition_name='{}' AND BINARY event_name='{}' "
        "AND from_expedition_uuid='{}' AND expire_time>NOW()",
        client.CharacterID(), database.Escape(Name), database.Escape(event_name),
        database.Escape(scope.dz->GetUUID())));
    return rows.Success() && rows.RowCount() == 1;
}

bool ReceiveDoomingExile(Client& client, Mob* caster)
{
    if (!caster || !caster->IsNPC() || !IsValidSpell(DoomingExile) || client.GetHP() <= 0) return false;
    auto* dragon = caster->CastToNPC();
    const auto scope = Resolve(client, End);
    if (!scope.dz || !scope.active || dragon->GetNPCTypeID() != Dragon ||
        entity_list.GetNPCByID(dragon->GetID()) != dragon || dragon->GetDepop() ||
        dragon->GetHP() <= 0 || !dragon->IsEngaged() ||
        dragon->GetEntityVariable(DragonTag) != scope.dz->GetUUID()) return false;
    const auto& spell = spells[DoomingExile];
    const auto delta = dragon->GetPosition() - client.GetPosition();
    if (spell.aoe_range <= 0.0f || delta.x * delta.x + delta.y * delta.y + delta.z * delta.z >
        spell.aoe_range * spell.aoe_range) return false;
    return dragon->SpellOnTarget(DoomingExile, &client, 0, true, spell.resist_difficulty);
}

bool ReservedHazard(const NPC& target)
{
    return zone && zone->GetZoneID() == Zone && zone->GetInstanceVersion() == 21 &&
        zone->GetInstanceID() && HazardType(target.GetNPCTypeID());
}

NPC* HazardTarget(Client& client)
{
    auto* mob = client.GetTarget();
    if (!mob || !mob->IsNPC()) return nullptr;
    auto* npc = mob->CastToNPC();
    if (!ReservedHazard(*npc) || client.GetClass() != Class::Rogue ||
        npc->GetClass() != Class::LDoNTreasure || npc->GetLoottableID() || npc->CountLoot() ||
        npc->GetDepop() || npc->GetHP() <= 0 || npc->IsAura() ||
        (npc->IsLDoNTrapped() && !IsValidSpell(npc->GetLDoNTrapSpellID()))) return nullptr;

    const auto scope = Resolve(client, End);
    if (!scope.dz || !scope.active || npc->GetEntityVariable(Hazard) != scope.dz->GetUUID() ||
        !npc->GetEntityVariable(DisarmedBy).empty()) return nullptr;
    const auto delta = client.GetPosition() - npc->GetPosition();
    if (delta.x * delta.x + delta.y * delta.y + delta.z * delta.z > RuleI(Adventure, LDoNTrapDistanceUse) ||
        !client.CheckLosFN(npc)) return nullptr;
    return npc;
}
}
