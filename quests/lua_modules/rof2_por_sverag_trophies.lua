local por = require("rof2_por_progression")
local M = {}

local rage_id = 374
local contact_spawn = 105981
local controller_condition = 1
local contacts = { [374001] = 1, [372099] = 2 }
local blocked_task = { [1] = 920327, [2] = 920326 }
local deliveries = {
    {920326, 88060, "bolvirk"},   {920327, 88061, "skulls"},
    {920328, 88062, "kobolds"},   {920329, 88063, "orcs"},
    {920330, 88064, "evil eyes"}, {920331, 88065, "wurines"},
    {920332, 88066, "drachnids"}, {920333, 88067, "shiliskins"},
    {920334, 88068, "berserkers"},
}

local function excluded(npc)
    local id = npc:GetNPCTypeID()
    local phase = contacts[id]
    if phase and por.public(npc, rage_id, id, contact_spawn)
        and eq.get_spawn_condition("rage", 0, controller_condition) == phase then
        return blocked_task[phase]
    end
end

function M.say(e)
    local excluded_task = excluded(e.self)
    if not excluded_task then return end

    local words = por.words(e)
    if words == "hail" then
        e.self:Say("I shall only hold my rage back if you [" .. eq.say_link("can prove", false) .. "] your worth.")
    end
    local asks_for_work = words == "can prove" or words == "i can prove my worth"
    local task_ids = {}
    for _, delivery in ipairs(deliveries) do
        local task, item, keyword = delivery[1], delivery[2], delivery[3]
        if task ~= excluded_task and (asks_for_work or words == keyword or
            (words == "hail" and e.other:CountItem(item) > 0)) then
            task_ids[#task_ids + 1] = task
        end
    end
    if #task_ids > 0 then
        e.other:TaskSelectorFromNPC(e.self, task_ids)
    end
end

function M.trade(e)
    local excluded_task = excluded(e.self)
    if excluded_task then
        for _, delivery in ipairs(deliveries) do
            local task, item = delivery[1], delivery[2]
            if task ~= excluded_task and e.other:IsTaskActivityActive(task, 0)
                and por.take(e, {[item] = 4}) then
                por.advance(e.other, task, 0, 4)
            end
        end
    end
    e.self:ReturnHandinItems(e.other)
end

function M.bolvirk_death(e)
    local npc = e.killed
    if not npc or not npc.valid or not por.current(rage_id, 0, 0) then return end
    local id = npc:GetNPCTypeID()
    if id ~= 372076 and id ~= 372085 and id ~= 372088 then return end

    local spawn = eq.get_entity_list():GetSpawnByID(npc:GetSpawnPointID())
    if spawn.valid and spawn:GetSpawnCondition() == controller_condition then
        local client = por.player(e.other)
        if client then por.advance(client, 920305, 1) end
    end
end

return M
