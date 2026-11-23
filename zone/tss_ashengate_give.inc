// Exact public native Give routes, used from the leased original trade writer.
namespace AshGive {

inline constexpr uint32_t Children = 920370;
inline constexpr uint32_t Pressure = 920372;
inline constexpr uint32_t Captured = 87187;
inline constexpr uint32_t Gland = 52610;
inline constexpr uint32_t Diamond = 87188;
inline constexpr char CaptureOwner[] = "tss_ashengate_capture_owner";
namespace Recipe = EQEmu::SharedCore::OriginalTradeRecipe;

bool Contact(const NPC& npc)
{
    return zone && zone->GetZoneID() == 405 && zone->GetInstanceVersion() == 0 &&
        zone->GetInstanceID() == 0 && npc.GetNPCTypeID() == 405153 && npc.GetSpawnPointID() == 113940;
}

bool Input(uint32_t id)
{
    return id == Captured || id == Gland || id == Diamond;
}

struct Plan {
    uint32_t task = 0;
    uint64_t accepted = 0;
    uint32_t mask = 0;
    Recipe::DebitPlan debit{};
    std::string Context() const
    {
        return fmt::format("tss-ashengate-give:{}:{}:{}", task, accepted, mask);
    }
};

uint64_t Accepted(ZoneDatabase& db, Client& c, uint32_t task, bool lock)
{
    auto rows = Q(db, fmt::format(
        "SELECT acceptedtime FROM character_tasks WHERE charid={} AND taskid={} "
        "AND acceptedtime>0 AND was_rewarded=0{}",
        c.CharacterID(), task, lock ? " FOR UPDATE" : ""));
    return rows.RowCount() == 1 ? N(rows.begin()[0]) : 0;
}

uint32_t Paid(ZoneDatabase& db, Client& client, const Plan& plan, bool lock)
{
    auto prefix = fmt::format("tss-ashengate-give:{}:{}:", plan.task, plan.accepted);
    auto rows=Q(db,fmt::format(
        "SELECT CAST(SUBSTRING(recipe_context,{}) AS UNSIGNED) FROM kithicor_poison_custody "
        "WHERE character_id={} AND recipe_kind={} AND physical=1 AND abort_only=0 "
        "AND npc_type_id=405153 AND npc_spawn_id=113940 AND instance_id=0 "
        "AND LEFT(recipe_context,{})=x'{}'{}",
        prefix.size()+1,client.CharacterID(),EQEmu::SharedCore::DodhExchange::JournalKind,
        prefix.size(),Hex(prefix),lock ? " FOR UPDATE" : ""));
    uint32_t mask = 0;
    for (auto row=rows.begin(); row!=rows.end(); ++row) {
        const auto value=N(row[0]);
        if (!value || value>(plan.task==Children ? 1u : 3u) || mask & value)
            throw std::runtime_error("Ashengate Give has conflicting receipts");
        mask|=static_cast<uint32_t>(value);
    }
    return mask;
}

Plan PlanGive(ZoneDatabase& db, Client& c, NPC& npc, const Recipe::Offers& offers)
{
    Plan plan;
    if (!Contact(npc) || c.IsSpiritShrouded()) return plan;
    auto engines=Q(db,
        "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
        "AND engine='InnoDB' AND table_name IN ('character_tasks','character_activities','kithicor_poison_custody')");
    if (engines.RowCount()!=1 || N(engines.begin()[0])!=3) return plan;
    const bool children=std::ranges::any_of(offers, [](const auto& offer) {
        return offer.present && offer.item_id==Captured;
    }) && c.IsTaskActivityActive(Children,2);
    plan.task=children ? Children : Pressure;
    plan.accepted=Accepted(db,c,plan.task,true);
    if (!plan.accepted) return plan;
    const auto paid=Paid(db,c,plan,true);
    auto collected=Q(db,fmt::format(
        "SELECT COUNT(*) FROM character_activities WHERE charid={} AND taskid={} "
        "AND activityid IN (0,1) AND completed=1 AND donecount={} FOR UPDATE",
        c.CharacterID(),plan.task,children ? 4 : 1));
    if (collected.RowCount()!=1 || N(collected.begin()[0])!=2) return plan;

    if (children) {
        if (paid & 1) return plan;
        uint32_t total=0;
        for (size_t index=0; index<offers.size() && total<4; ++index) {
            const auto& offer=offers[index];
            if (!offer.present || offer.item_id!=Captured || offer.stackable) continue;
            auto* item=c.GetInv().GetItem(offer.slot);
            if (!item || item->GetCustomData(CaptureOwner)!=std::to_string(c.CharacterID())) continue;
            plan.debit.units[index]=1;
            ++total;
        }
        if (total==4) plan.mask=1;
        else plan.debit={};
        return plan;
    }

    for (const auto& [id,activity,bit] : {std::tuple{Gland,2u,1u},std::tuple{Diamond,3u,2u}}) {
        if ((paid&bit) || !c.IsTaskActivityActive(Pressure,activity)) continue;
        Recipe::DebitPlan one;
        if (!Recipe::FirstUnit(offers,id,one)) continue;
        for (size_t index=0; index<one.units.size(); ++index) plan.debit.units[index]+=one.units[index];
        plan.mask|=bit;
    }
    return plan;
}

bool Reconcile(Client& client)
{
    for (const auto task : {Children,Pressure}) {
        if (!client.IsTaskActive(task)) continue;
        Plan plan{task,Accepted(database,client,task,false)};
        if (!plan.accepted) continue;
        const auto paid=Paid(database,client,plan,false);
        if ((paid&1) && client.IsTaskActivityActive(task,2))
            client.UpdateTaskActivity(task,2,task==Children ? 4 : 1);
        if (task==Pressure && (paid&2) && client.IsTaskActivityActive(task,3))
            client.UpdateTaskActivity(task,3,1);
    }
    return true;
}
}
