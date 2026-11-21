local por=require("rof2_por_progression")
local scene=require("rof2_por_scene_state")
local M={}
local samples=920306
local vial=84156
local shadow=375960
local room_kills, shadow_id=0, nil

local function combatant(npc,min,max)
    return npc and npc.valid and npc:GetNPCTypeID()>=min and npc:GetNPCTypeID()<=max and npc:GetSpawnPointID()>0
end

function M.arrive(client)
    if por.inside(client,samples,375,10) then por.advance(client,samples,0) end
end

function M.tower_death(e)
    local client=por.player(e.other)
    local npc=e.killed
    if not client or not combatant(npc,375001,375065) or not por.inside(client,samples,375,10) or
        not client:IsTaskActivityActive(samples,1) then return end
    local placement=npc:GetSpawnPointID()
    if e.corpse and e.corpse.valid and scene.start(client,samples,375,10,placement,1) then
        -- Stable selection across zone restarts. Only the original death of a
        -- particular retained placement in this owned run can award a sample.
        local run=tonumber(client:GetSharedTaskIdentity()) or 0
        if (placement+run)%3==0 and e.corpse:CountItem(vial)==0 then
            e.corpse:AddItem(vial,1)
            -- Empty NPC corpses have a short timer set before the death callback.
            if e.corpse:CountItem(vial)==1 then e.corpse:ResetDecayTimer() end
        end
    end
    room_kills=room_kills+1
    if room_kills%8~=0 then return end
    if shadow_id then
        local prior=eq.get_entity_list():GetNPCByID(shadow_id)
        if prior.valid then return end
    end
    local mob=eq.spawn2(shadow,0,0,npc:GetX(),npc:GetY(),npc:GetZ(),npc:GetHeading())
    if mob.valid and mob:IsNPC() then
        local attacker=mob:CastToNPC()
        shadow_id=attacker:GetID()
        attacker:AddToHateList(client,1)
        attacker:CastSpell(8762,client:GetID())
    end
end

function M.sverag_death(e)
    local client=por.player(e.other)
    if client and combatant(e.killed,374002,374299) and por.current(374,0,0) then
        por.advance(client,920305,1)
    end
end

return M
