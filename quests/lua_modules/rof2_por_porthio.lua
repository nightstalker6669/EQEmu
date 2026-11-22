local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local M = {}
local actors = {
    [371941] = {spawn=304087090, spells={8612}, interval=32000},
    [371942] = {spawn=304087091, spells={8556,8964}, interval=28000},
}
local timer = "por_relic_combat"
local pursuit_timer = "por_nightshade_pursuit"
local pursuit

local function actor(npc)
    local row = actors[npc:GetNPCTypeID()]
    if row and por.current(370,0,0) and npc:GetSpawnPointID() == row.spawn then return row end
end

function M.combat(e)
    local npc = e.self
    local row = actor(npc)
    if not row then return end
    if e.joined ~= true and tonumber(e.joined) ~= 1 then eq.stop_timer(timer,npc); return end
    eq.set_timer(timer,row.interval,npc)
    if npc:GetNPCTypeID() ~= 371941 or not por.player(e.other) then return end
    for other in eq.get_entity_list():GetNPCList().entries do
        if other:GetNPCTypeID() == 371942 and actor(other) and not other:IsEngaged() then
            other:AddToHateList(e.other,1)
            return
        end
    end
end

function M.timer(e)
    local npc = e.self
    local row = actor(npc)
    if not row then return end
    if e.timer == pursuit_timer then
        local chase = pursuit
        local target = chase and eq.get_entity_list():GetClientByCharID(chase.target)
        if not chase or chase.npc ~= npc:GetID() or not target.valid or target:GetHP() <= 0 then
            if chase and chase.npc == npc:GetID() then pursuit = nil end
            npc:SetSpecialAbility(28, 0)
            eq.stop_timer(pursuit_timer,npc)
            return
        end
        local top = npc:GetHateTop()
        if chase.locked or npc:CalculateDistance(target) <= 15 then
            chase.locked = true
            npc:SetSpecialAbility(28, 1)
            if not top.valid or top:GetID() ~= target:GetID() then
                npc:WipeHateList()
                npc:AddToHateList(target, 1000000)
            end
        elseif top.valid and top:GetID() ~= target:GetID() then
            pursuit = nil
            eq.stop_timer(pursuit_timer,npc)
        end
        return
    end
    if e.timer ~= timer then return end
    for _, spell in ipairs(row.spells) do npc:SpellFinished(spell,npc) end
    if npc:GetNPCTypeID() ~= 371941 or not npc:IsEngaged() then return end
    if pursuit then
        local shade = eq.get_entity_list():GetNPCByID(pursuit.npc)
        if shade.valid and actor(shade) and shade:GetNPCTypeID() == 371942 then return end
        pursuit = nil
    end
    local targets = {}
    for client in eq.get_entity_list():GetClientList().entries do
        if client:GetHP() > 0 and npc:CalculateDistance(client) <= 180 then targets[#targets+1] = client end
    end
    if #targets == 0 then return end
    for shade in eq.get_entity_list():GetNPCList().entries do
        if shade:GetNPCTypeID() == 371942 and actor(shade) then
            local target = targets[math.random(#targets)]
            npc:Shout("Nightshade, to me! Hunt them down!")
            shade:WipeHateList()
            shade:AddToHateList(target,1)
            pursuit = {npc=shade:GetID(),target=target:CharacterID(),locked=false}
            eq.set_timer(pursuit_timer,1000,shade)
            return
        end
    end
end

return M
