// Public originals, included by the native inventory writer.
namespace AshPublic {
inline constexpr uint32_t Sabotage = 920367;
inline constexpr uint32_t Children = 920370;
inline constexpr uint32_t Captured = 87187;
inline constexpr uint32_t Explosives = 87199;
inline constexpr int32_t Detonation = 2490;

bool PublicAshengate()
{
    return zone && zone->GetZoneID() == TssAshengate::ZoneID &&
           zone->GetInstanceVersion() == 0 && zone->GetInstanceID() == 0;
}

bool SabotageSite(Client& client)
{
    const auto dx=client.GetX()+719.0f;
    const auto dy=client.GetY()-960.0f;
    return PublicAshengate() && !client.IsSpiritShrouded() && client.GetHP()>0 &&
        client.IsTaskActivityActive(Sabotage,4) && dx*dx+dy*dy<=144.0f &&
        std::abs(client.GetZ()+39.903f)<=8.0f;
}

bool Egg(const NPC& target)
{
    switch (target.GetSpawnPointID()) {
    case 113096: return target.GetNPCTypeID()==406013;
    case 113099:
    case 113107:
    case 113112: return target.GetNPCTypeID()==406015;
    case 113137: return target.GetNPCTypeID()==406034;
    default: return false;
    }
}

bool Token(std::string_view value)
{
    return value.size()==64 && value.find_first_not_of("0123456789ABCDEFabcdef")==std::string_view::npos;
}

std::string Nonce()
{
    uint8_t bytes[32];
    return RAND_bytes(bytes,sizeof(bytes))==1 ? Hex(bytes,sizeof(bytes)) : std::string{};
}

bool Active(Client& c, uint32_t task, uint8_t activity)
{
    return PublicAshengate() && !c.IsSpiritShrouded() && c.GetHP()>0 &&
        c.IsTaskActivityActive(task,activity);
}

uint64_t ActionCount(ZoneDatabase& db, Client& c, uint32_t task, uint64_t accepted, uint32_t activity)
{
    auto rows=Q(db,fmt::format(
        "SELECT COUNT(*) FROM tss_ashengate_public_actions WHERE character_id={} "
        "AND task_id={} AND accepted_time={} AND activity_id={}",c.CharacterID(),task,accepted,activity));
    if (rows.RowCount()!=1) throw std::runtime_error("Ashengate action count unavailable");
    return N(rows.begin()[0]);
}

void Acknowledge(Client& client, uint32_t task, uint32_t activity, uint32_t maximum)
{
    if (!client.IsTaskActivityActive(task,activity)) return;
    const auto accepted=AshGive::Accepted(database,client,task,false);
    if (!accepted) return;
    const auto count=ActionCount(database,client,task,accepted,activity);
    const auto done=client.GetTaskActivityDoneCountFromTaskID(task,activity);
    if (done>=0 && count<=maximum && count>static_cast<uint64_t>(done))
        client.UpdateTaskActivity(task,activity,static_cast<int>(count)-done);
}

struct Capture {
    std::string token;
    uint64_t accepted = 0;
    uint32_t hatch_owner = 0;
    uint64_t hatch_accepted = 0;
};

bool ValidateBaby(ZoneDatabase& db, Client& client, NPC& baby, const Capture& proof, bool lock)
{
    using namespace TssAshengate;
    if (!Active(client,Children,1) || baby.GetNPCTypeID()!=Baby || baby.GetDepop() || baby.GetHP()<=0 ||
        entity_list.GetNPCByID(baby.GetID())!=&baby || !Token(proof.token) ||
        baby.GetEntityVariable(BabyToken)!=proof.token ||
        baby.GetEntityVariable(BabyOwner)!=std::to_string(proof.hatch_owner) ||
        baby.GetEntityVariable(BabyOccurrence)!=std::to_string(proof.hatch_accepted) ||
        AshGive::Accepted(db,client,Children,lock)!=proof.accepted) return false;
    if (proof.hatch_owner != client.CharacterID()) {
        auto* owner = entity_list.GetClientByCharID(proof.hatch_owner);
        if (!owner || !client.GetGroup() || owner->GetGroup() != client.GetGroup() ||
            !owner->IsTaskActive(Children) || AshGive::Accepted(db,*owner,Children,lock)!=proof.hatch_accepted)
            return false;
    }
    auto hatch=Q(db,fmt::format(
        "SELECT placement_id FROM tss_ashengate_public_actions WHERE source_token='{}' "
        "AND activity_id=0 AND task_id={} AND character_id={} AND accepted_time={}{}",
        proof.token,Children,proof.hatch_owner,proof.hatch_accepted,lock ? " FOR UPDATE" : ""));
    return hatch.RowCount()==1;
}
}
