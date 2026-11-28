local ring = {}
local contender, bastiz, sisrira, blade = 763960, 763961, 763962, 763963
local contender_spawn = 304101211
local successor = {[contender] = bastiz, [bastiz] = sisrira, [sisrira] = blade}
local predecessor = {[bastiz] = contender, [sisrira] = bastiz}
local origin_variable = "may_hunter_chelsith_parent"
local merit_variable = "may_hunter_chelsith_merit"

local function belongs(npc)
    if eq.get_zone_id() ~= 763 or eq.get_zone_instance_version() ~= 0
        or eq.get_zone_instance_id() ~= 0 or not npc.valid or npc:IsPet() then
        return false
    end
    local npc_type = npc:GetNPCTypeID()
    if npc_type == contender then
        return npc:GetSpawnPointID() == contender_spawn
    end
    local parent = predecessor[npc_type]
    return parent and npc:GetSpawnPointID() == 0
        and npc:GetEntityVariable(origin_variable) == tostring(parent)
end

local function ring_in_progress()
    local entities = eq.get_entity_list()
    for _, npc_type in ipairs({bastiz, sisrira, blade}) do
        local npc = entities:GetNPCByNPCTypeID(npc_type)
        if npc.valid and npc:GetHP() > 0 then
            return true
        end
    end
    return false
end

function ring.spawn(e)
    if belongs(e.self) and ring_in_progress() then
        e.self:Depop(true)
    end
end

function ring.killed_merit(e)
    if belongs(e.self) and e.other and e.other.valid and e.other:IsClient() then
        e.self:SetEntityVariable(merit_variable, "1")
    end
end

function ring.death_complete(e)
    local npc = e.self
    if not belongs(npc) or npc:GetEntityVariable(merit_variable) ~= "1" then
        return
    end
    npc:SetEntityVariable(merit_variable, "")
    local npc_type = npc:GetNPCTypeID()
    local next_type = successor[npc_type]
    if not next_type or ring_in_progress() then
        return
    end
    -- Keep the public ring at the arena marker if a combatant was pulled away.
    local mob = eq.unique_spawn(next_type, 0, 0, 3.7845, 423.3073, 267.2374, 256)
    if mob.valid and mob:IsNPC() then
        local next_npc = mob:CastToNPC()
        if next_npc:GetNPCTypeID() == next_type and next_npc:GetSpawnPointID() == 0 then
            next_npc:SetEntityVariable(origin_variable, tostring(npc_type))
        end
    end
end

return ring
