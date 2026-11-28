local defector_type = 725998
local defector_spawn = 304101407
local krongar_type = 725999
local merit_key = "may_hunter_krongar_defector_merit"

local function is_public_defector(npc)
    return eq.get_zone_id() == 725 and eq.get_zone_instance_version() == 0
        and eq.get_zone_instance_id() == 0 and npc.valid and not npc:IsPet()
        and npc:GetNPCTypeID() == defector_type and npc:GetSpawnPointID() == defector_spawn
end

function event_killed_merit(e)
    if is_public_defector(e.self) and e.other and e.other.valid and e.other:IsClient() then
        e.self:SetEntityVariable(merit_key, "1")
    end
end

function event_death_complete(e)
    local npc = e.self
    if not is_public_defector(npc) or npc:GetEntityVariable(merit_key) ~= "1" then
        return
    end

    npc:SetEntityVariable(merit_key, "")
    local existing = eq.get_entity_list():GetNPCByNPCTypeID(krongar_type)
    if existing.valid and existing:GetHP() > 0 then
        return
    end
    -- The original defector kill is sourced. The local rare roll follows comparable camps.
    if math.random(100) <= 10 then
        eq.unique_spawn(krongar_type, 0, 0, 698.2133, -250.2952, -46.2631, 256)
    end
end
