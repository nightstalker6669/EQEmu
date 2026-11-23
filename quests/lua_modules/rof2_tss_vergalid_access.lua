local route = require("rof2_por_progression")
local layout = require("rof2_tss_vergalid_layout")
local read_one = require("rof2_dodh_scene_receipts").read_one

local M = {}

local function completed_shared(client, task)
    return not not read_one([[
        SELECT 1
        FROM completed_shared_tasks t
        JOIN completed_shared_task_members m ON m.shared_task_id = t.id
        WHERE t.task_id = ? AND m.character_id = ? AND t.completion_time IS NOT NULL
        LIMIT 1
    ]], {task, client:CharacterID()})
end

local function zheren_finished(client)
    for task = 3040702, 3040705 do
        if not client:IsTaskCompleted(task) then return false end
    end
    return true
end

local function kel_finished(client)
    return completed_shared(client, layout.kellak.task)
end

local function is_leader(client)
    local raid = client:GetRaid()
    if raid.valid then return raid:IsLeader(client) end
    local group = client:GetGroup()
    return group.valid and group:IsLeader(client)
end

local function six_present(client)
    local raid = client:GetRaid()
    local count = 0
    if raid.valid then
        for index = 0, 71 do
            local member = raid:GetMember(index)
            if member.valid then count = count + 1 end
            if count >= 6 then return true end
        end
        return false
    end

    local group = client:GetGroup()
    if not group.valid then return false end
    for index = 0, 5 do
        local member = group:GetMember(index)
        if member.valid and member:IsClient() then count = count + 1 end
    end
    return count == 6
end

function M.kazzar(e)
    if not route.public(e.self, 403, 403095, 114212) or not e.other.valid then return end
    local client, words = e.other, route.words(e)
    if words == "hail" or words == "hail, " .. e.self:GetCleanName():lower() then
        e.self:Say("Our enemies wait in the " .. eq.say_link("Vergalid Mines") .. " to the east.")
        return
    end
    if words == "what vergalid mines" then words = "vergalid mines" end
    if words == "i am willing" then words = "willing" end
    if words ~= "vergalid mines" and words ~= "willing" and words ~= "challenge" then return end
    if not zheren_finished(client) then
        e.self:Say("First complete Captain Zheren’s work scouting the enemy and disrupting the mines.")
        return
    end
    if words == "vergalid mines" then
        local choice = kel_finished(client) and (" You can also " .. eq.say_link("challenge") .. " Vergalid.") or ""
        e.self:Say("Are you " .. eq.say_link("willing") .. " to strike at Kellak?" .. choice)
        return
    end
    if client:GetLevel() < 75 then
        e.self:Say("Your force needs an experienced leader to take on the mines.")
        return
    end
    if not is_leader(client) then
        e.self:Say("Have your group or raid leader speak to me.")
        return
    end

    local task = layout.kellak.task
    if words == "challenge" then
        if not client:GetRaid().valid then
            e.self:Say("Assemble your strike force as a raid before you challenge Vergalid.")
            return
        end
        if not kel_finished(client) then
            e.self:Say("Defeat Kellak before you challenge Vergalid.")
            return
        end
        if not six_present(client) then
            e.self:Say("Gather at least six of your force here in Sunderock Springs.")
            return
        end
        task = layout.ending.task
    end
    client:TaskSelectorFromNPC(e.self, {task})
end

function M.entrance(e)
    if not route.current(403, 0, 0) or not e.door.valid then return 0 end
    local binding
    if e.door:GetDoorID() == 2 and e.door:GetDoorDBID() == 17359 then binding = layout.kellak end
    if e.door:GetDoorID() == 3 and e.door:GetDoorDBID() == 17360 then binding = layout.ending end
    if not binding then return 0 end
    local player = e.self
    if not player.valid then return 1 end
    if not route.enter(player, binding.task, 404, binding.version, 14, 0, 3, 0) then
        player:Message(13, "You are not assigned to the raid beyond this entrance.")
    end
    return 1
end

return M
