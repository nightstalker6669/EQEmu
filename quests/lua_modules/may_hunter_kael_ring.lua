local ring = {}
local statue, idol, avatar = 754960, 754961, 754962
local next_actor = {[statue] = idol, [idol] = avatar}
local armor_spawn = 154952
local provenance = "may_hunter_kael_parent"
local merit = "may_hunter_kael_merit"

local function public_kael()
    return eq.get_zone_id() == 754 and eq.get_zone_instance_version() == 0
        and eq.get_zone_instance_id() == 0
end

local function active(type_id)
    local mob = eq.get_entity_list():GetNPCByNPCTypeID(type_id)
    return mob.valid and mob:GetHP() > 0
end

local function belongs(npc)
    if not public_kael() or not npc.valid or npc:IsPet() then
        return false
    end
    local type_id = npc:GetNPCTypeID()
    if type_id == statue then
        return npc:GetSpawnPointID() == armor_spawn
    end
    local expected_parent = type_id == idol and statue or type_id == avatar and idol
    return expected_parent and npc:GetSpawnPointID() == 0
        and npc:GetEntityVariable(provenance) == tostring(expected_parent)
end

function ring.spawn(e)
    if belongs(e.self) and (active(idol) or active(avatar)) then
        e.self:Depop(true)
    end
end

function ring.killed_merit(e)
    if belongs(e.self) and e.other and e.other.valid and e.other:IsClient() then
        e.self:SetEntityVariable(merit, "1")
    end
end

function ring.death_complete(e)
    local npc = e.self
    if not belongs(npc) or npc:GetEntityVariable(merit) ~= "1" then
        return
    end
    npc:SetEntityVariable(merit, "")
    local type_id = npc:GetNPCTypeID()
    local successor = next_actor[type_id]
    if not successor or active(idol) or active(avatar) then
        return
    end
    -- Each stage returns to the existing Armor of War camp, even if pulled.
    local mob = eq.unique_spawn(successor, 0, 0, 1374.8566, 2761.0872, -65.5986, 255)
    if mob.valid and mob:IsNPC() then
        local successor_npc = mob:CastToNPC()
        if successor_npc:GetNPCTypeID() == successor and successor_npc:GetSpawnPointID() == 0 then
            successor_npc:SetEntityVariable(provenance, tostring(type_id))
        end
    end
end

function ring.combat(e)
    if e.joined and belongs(e.self) then
        e.self:CastSpell(32982, e.self:GetID())
    end
end

return ring
