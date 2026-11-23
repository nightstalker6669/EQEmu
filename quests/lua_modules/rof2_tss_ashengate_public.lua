local route = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")

local M = {}
local sabotage, disarm, rescue = 920367, 920368, 920371
local crystal = {-719, 960, -39.903}
local weapons = {
    [113221] = 406081, [113245] = 406081, [113261] = 406081, [113277] = 406081,
    [113244] = 406090, [113299] = 406090, [113301] = 406090,
}
local arbiter = {[113332] = 406115, [113333] = 406031}
local hidaga = 406937
local hidaga_point = {730, 1370, -136.03, 384}
local rescue_occurrence
local timer = "tss_ashengate_sabotage"

local function public_zone()
    return route.current(406, 0, 0)
end

local function party(killer, npc, visit)
    local group = killer:GetGroup()
    if not group.valid then
        visit(killer)
        return
    end
    for index = 0, 5 do
        local member = group:GetMember(index)
        if member.valid and member:IsClient() and member:GetHP() > 0
            and member:CalculateDistance(npc:GetX(), npc:GetY(), npc:GetZ()) <= 200 then
            visit(member:CastToClient())
        end
    end
end

local function destroy_weapons(client, count)
    for _ = 1, count do
        local active = {}
        for activity = 0, 2 do
            if client:IsTaskActivityActive(disarm, activity) then active[#active + 1] = activity end
        end
        if #active == 0 then return end
        route.advance(client, disarm, active[math.random(#active)])
    end
end

function M.death(e)
    if not public_zone() or not e.killed or not e.killed.valid
        or not e.killed:IsNPC() then return end
    local killer = route.player(e.other)
    if not killer then return end
    local npc = e.killed:CastToNPC()
    local kind, point = npc:GetNPCTypeID(), npc:GetSpawnPointID()
    if weapons[point] == kind then
        local count = math.random(5, 6)
        party(killer, npc, function(member) destroy_weapons(member, count) end)
        return
    end
    if arbiter[point] ~= kind then return end
    local trigger = false
    party(killer, npc, function(member)
        if member:IsTaskActivityActive(rescue, 0) then trigger = true end
    end)
    if not trigger or eq.get_entity_list():GetNPCByNPCTypeID(hidaga).valid then return end

    local spawn = runtime.spawn_owned(hidaga, hidaga_point, "tss_ashengate:rescue")
    if spawn then
        rescue_occurrence = {id = spawn:GetID(), opened = os.time(), uses = 0, hailed = {}}
    end
end

function M.hidaga_spawn(e)
    if public_zone() then eq.set_timer("hidaga_leaves", 30000) end
end

function M.hidaga_timer(e)
    if e.timer ~= "hidaga_leaves" then return end
    eq.stop_timer(e.timer)
    if rescue_occurrence and rescue_occurrence.id == e.self:GetID() then rescue_occurrence = nil end
    e.self:Depop(false)
end

function M.hidaga_say(e)
    local client, npc, state = e.other, e.self, rescue_occurrence
    if not public_zone() or not client.valid or not state or state.id ~= npc:GetID()
        or npc:GetNPCTypeID() ~= hidaga or state.uses >= 12 or os.time() - state.opened >= 30
        or state.hailed[client:CharacterID()] then return end
    local words = route.words(e)
    if words ~= "hail" and words ~= "hail, hidaga" then return end
    if not route.advance(client, rescue, 0) then return end
    state.hailed[client:CharacterID()] = true
    state.uses = state.uses + 1
    npc:Say("Thank you for helping me escape. Tell Jenray what happened.")
    if state.uses == 12 then
        rescue_occurrence = nil
        npc:Depop(false)
    end
end

function M.enter(e)
    if public_zone() and e.self:IsTaskActive(sabotage) then eq.set_timer(timer, 1000) end
end

function M.tick(e)
    if e.timer ~= timer or not public_zone() then return end
    local client = e.self
    if not client:IsTaskActive(sabotage) then eq.stop_timer(timer); return end
    if not client:IsTaskActivityActive(sabotage, 4) then return end
    local dx, dy = client:GetX() - crystal[1], client:GetY() - crystal[2]
    if dx * dx + dy * dy <= 144 and math.abs(client:GetZ() - crystal[3]) <= 8 then
        client:CommitAshengateSabotage()
    end
end

function M.combine(e)
    if e.recipe_id == 304097510
        and (not public_zone() or not e.self:IsTaskActivityActive(sabotage, 3)) then
        return 1
    end
    return 0
end

return M
