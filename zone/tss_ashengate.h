#pragma once

#include "common/faction.h"
#include "common/shared_task_admission.h"
#include "zone/client.h"
#include "zone/dynamic_zone.h"
#include "zone/npc.h"
#include "zone/zone.h"
#include "zone/zonedb.h"

#include <array>
#include <charconv>
#include <string_view>
#include <optional>

class Corpse;
class NPC;

namespace TssAshengate {
enum class LootResult { Unrelated, Refused, Issued, AlreadyIssued, InventoryReload };
void CaptureMarkCorpse(Corpse& corpse, NPC& source);
bool ClaimsGive(Client& client, NPC& npc);
bool ReconcileGive(Client& client);
LootResult LootMark(Client& client, Corpse& corpse, const EQ::ItemInstance& item);

inline constexpr uint32_t ZoneID = 406;
inline constexpr uint32_t Mark = 87292;
inline constexpr uint16_t MarkSpell = 11653;
inline constexpr uint32_t Incubator = 87185;
inline constexpr uint16_t IncubatorSpell = 9163;
inline constexpr uint32_t Baby = 406936;
inline constexpr char BabyOwner[] = "tss_ashengate:baby_owner";
inline constexpr char BabyOccurrence[] = "tss_ashengate:baby_occurrence";
inline constexpr char BabyToken[] = "tss_ashengate:baby_token";
inline constexpr char MarkSource[] = "tss_ashengate:mark_source";
inline constexpr char MarkToken[] = "tss_ashengate_mark_token";
inline constexpr char MarkOwner[] = "tss_ashengate_mark_owner";

// The stock Mark is instant. This scope exists only while the original
// packet dispatches that exact item spell; no script can construct it.
class OriginalClickScope {
public:
    OriginalClickScope(Client& client, int16_t slot, uint32_t spell, uint32_t target);
    ~OriginalClickScope();
    OriginalClickScope(const OriginalClickScope&) = delete;
    OriginalClickScope& operator=(const OriginalClickScope&) = delete;
    bool Allowed() const { return allowed_; }
    static bool Matches(Client& client, int16_t slot);
    static bool BeginIncubator(Client& client, int16_t slot, uint32_t target);
private:
    inline static thread_local const OriginalClickScope* active_ = nullptr;
    const OriginalClickScope* previous_;
    Client* client_;
    int16_t slot_;
    int32_t serial_ = 0;
    uint32_t target_ = 0;
    uint32_t spell_ = 0;
    bool allowed_ = true;
};

bool ConsumeMark(Client& client, int16_t slot);
bool AllowsIncubatorLanding(Client& client, int16_t slot, NPC& actor);
void CancelIncubator(Client& client);
void FinishIncubator(Client& client, int16_t slot, uint32_t target);
bool CommitSabotage(Client& client);


struct Wing {
    uint32_t version;
    const char* name;
    int32_t faction;
    uint32_t side_task;
    uint32_t contact;
    uint32_t placement;
};

inline constexpr std::array Wings{
    Wing{20, "Ashengate East", 1135, 920362, 405092, 113693},
    Wing{21, "Ashengate West", 1134, 920363, 405153, 113940}
};

struct Reward {
    uint32_t npc;
    uint32_t version;
    uint8_t event;
    uint8_t mark_count;
    const char* name;
};

inline constexpr std::array Rewards{
    Reward{406938,20,1,1,"Griffon Master Ayna'Rym"},
    Reward{406948,20,2,1,"Stitch's Ambush"},
    Reward{406953,20,3,2,"Stitch"},
    Reward{406961,21,4,1,"Ambersnout"},
    Reward{406972,21,5,1,"Attendants of Sothgar"},
    Reward{406979,21,6,2,"Sothgar the Twiceborn"}
};

inline const Wing* FindWing(uint32_t version)
{
    for (const auto& wing : Wings) if (wing.version == version) return &wing;
    return nullptr;
}

inline const Reward* FindReward(uint32_t npc)
{
    for (const auto& reward : Rewards) if (reward.npc == npc) return &reward;
    return nullptr;
}

inline DynamicZone* CurrentWing(const Wing& wing)
{
    if (!zone || zone->GetZoneID() != ZoneID || zone->GetInstanceVersion() != wing.version ||
        !zone->GetInstanceID()) return nullptr;
    auto* dz = zone->GetDynamicZone();
    return dz && dz->IsCurrentZoneDz() && dz->GetType() == DynamicZoneType::Expedition &&
        dz->IsValid() && !dz->IsExpired() && dz->GetName() == wing.name &&
        dz->GetUUID().size() == 36 ? dz : nullptr;
}

inline std::optional<int32_t> SignedNumber(const char* value)
{
    if (!value) return std::nullopt;
    const std::string_view text(value);
    int32_t result{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()
        ? std::optional(result) : std::nullopt;
}

inline bool FactionMeets(const Wing& wing, int32_t standing, uint32_t class_id, uint32_t race,
    uint32_t deity, bool kindly)
{
    FactionMods fixed{};
    if (!content_db.GetFactionData(&fixed, class_id, race, deity, wing.faction)) return false;
    const auto value = standing + fixed.base + fixed.class_mod + fixed.race_mod + fixed.deity_mod;
    return value >= (kindly ? RuleI(Faction, KindlyFactionMinimum) : RuleI(Faction, AllyFactionMinimum));
}

inline bool HasFaction(Client& client, uint32_t version, bool kindly)
{
    const auto* wing = FindWing(version);
    if (!wing || client.IsSpiritShrouded()) return false;
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT current_value FROM faction_values WHERE char_id={} AND faction_id={} AND temp=0",
        client.CharacterID(), wing->faction));
    if (!rows.Success() || rows.RowCount() > 1) return false;
    const auto standing = rows.RowCount() ? SignedNumber(rows.begin()[0]) : std::optional<int32_t>{0};
    return standing && FactionMeets(*wing, *standing, client.GetBaseClass(), client.GetBaseRace(),
        client.GetDeity(), kindly);
}

inline bool HasMark(Client& client)
{
    if (client.IsSpiritShrouded()) return false;
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT 1 FROM tss_ashengate_marks WHERE character_id={} AND consumed_at>0",
        client.CharacterID()));
    return rows.Success() && rows.RowCount() == 1;
}

// c is a typed character_data alias supplied by the native admission queries.
inline constexpr char VergalidWon[] =
    "EXISTS(SELECT 1 FROM completed_shared_tasks t "
    "JOIN completed_shared_task_members m ON m.shared_task_id=t.id "
    "JOIN completed_shared_task_activity_state a ON a.shared_task_id=t.id "
    "WHERE t.task_id=920361 AND t.completion_time IS NOT NULL AND m.character_id=c.id "
    "AND a.activity_id=4 AND a.done_count=1 AND a.completed_time IS NOT NULL)";
inline constexpr char MarkConsumed[] =
    "EXISTS(SELECT 1 FROM tss_ashengate_marks mark WHERE mark.character_id=c.id AND mark.consumed_at>0)";

inline bool CanRequest(Client& client, const Wing& wing)
{
    if (!zone || zone->GetZoneID() != 405 || zone->GetInstanceVersion() != 0 ||
        zone->GetInstanceID() || client.IsSpiritShrouded()) return false;
    auto* target = client.GetTarget();
    if (!target || !target->IsNPC()) return false;
    auto* contact = target->CastToNPC();
    if (contact->GetNPCTypeID() != wing.contact || contact->GetSpawnPointID() != wing.placement)
        return false;
    for (uint32_t task = 3040702; task <= 3040705; ++task) {
        if (!client.IsTaskCompleted(task)) return false;
    }
    if (!client.IsTaskCompleted(wing.side_task) || !HasFaction(client, wing.version, false)) return false;
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT c.id FROM character_data c WHERE c.id={} AND ({} OR {}) "
        "AND EXISTS(SELECT 1 FROM completed_shared_tasks t JOIN completed_shared_task_members m "
        "ON m.shared_task_id=t.id WHERE t.task_id=920360 AND t.completion_time IS NOT NULL "
        "AND m.character_id=c.id)", client.CharacterID(), VergalidWon, MarkConsumed));
    return rows.Success() && rows.RowCount() == 1;
}

inline bool CanEnter(Client& client, const Wing& wing, uint32_t instance)
{
    if (!instance || client.IsSpiritShrouded()) return false;
    auto rows = database.QueryDatabase(fmt::format(
        "SELECT d.uuid,m.character_id,c.level,c.class,c.race,c.deity,COALESCE(f.current_value,0),"
        "({} OR {}),ip.charid FROM instance_list i "
        "JOIN dynamic_zones d ON d.instance_id=i.id "
        "JOIN dynamic_zone_members m ON m.dynamic_zone_id=d.id "
        "LEFT JOIN instance_list_player ip ON ip.id=i.id AND ip.charid=m.character_id "
        "LEFT JOIN character_data c ON c.id=m.character_id "
        "LEFT JOIN faction_values f ON f.char_id=c.id AND f.faction_id={} AND f.temp=0 "
        "WHERE i.id={} AND i.zone=406 AND i.version={} AND i.expire_at>UNIX_TIMESTAMP() "
        "AND d.type={} AND BINARY d.name='{}' ORDER BY m.character_id",
        VergalidWon, MarkConsumed, wing.faction, instance, wing.version,
        static_cast<uint32_t>(DynamicZoneType::Expedition), wing.name));
    if (!rows.Success() || !rows.RowCount() || rows.RowCount() > 54) return false;

    using SharedTaskAdmission::Number;
    uint32_t qualified = 0;
    uint64_t previous = 0;
    bool present = false;
    bool independent = false;
    for (auto row = rows.begin(); row != rows.end(); ++row) {
        const auto id = Number(row[1]);
        const auto level = Number(row[2]);
        const auto class_id = Number(row[3]);
        const auto race = Number(row[4]);
        const auto deity = Number(row[5]);
        const auto standing = SignedNumber(row[6]);
        const auto equivalent = Number(row[7]);
        if (!row[0] || std::string_view(row[0]).size() != 36 || !id || *id <= previous ||
            Number(row[8]) != id || !level || !*level || !class_id || !race || !deity ||
            !standing || !equivalent || *equivalent > 1) return false;
        previous = *id;
        const auto can_attend = *level >= 74 || (*equivalent && FactionMeets(wing, *standing,
            static_cast<uint32_t>(*class_id), static_cast<uint32_t>(*race), static_cast<uint32_t>(*deity), false));
        if (can_attend) ++qualified;
        if (*id == client.CharacterID()) {
            present = true;
            independent = can_attend;
        }
    }
    return present && (independent || 100 * qualified >= 85 * rows.RowCount());
}
}
