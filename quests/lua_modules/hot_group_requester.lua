local missions = {
    [700075] = { 920380, "instrument of their destruction", "depart" },
    [701022] = { 920381, "ready", "go" },
    [703044] = { 920382, "stop", "ready" },
    [705004] = { 920383, "help", "ready" },
    [704000] = { 920384, "confront", "proceed" },
    [706047] = { 920385, "willing", "leave" },
    [702000] = { 920386, "stop", "know" },
    [702051] = { 920387, "enter", "proceed" },
    [710047] = { 920388, "master", "damage" },
    [711084] = { 920389, "meet", "begin" }
}

local M = {}

function M.say(e, show_hail)
    local mission = missions[e.self:GetNPCTypeID()]
    if not mission or not e.other.valid then
        return false
    end

    local task_id, request, enter = unpack(mission)
    local message = e.message:lower()
    local has_mission = e.other:IsTaskActive(task_id)
    if message == enter and has_mission then
        if not e.other:EnterHouseOfThuleGroupMission(task_id, e.self:GetID()) then
            e.self:Say("Your group's passage is unavailable. Confirm that the mission is active.")
        end
    elseif message == request and not has_mission then
        eq.task_selector({ task_id })
    elseif show_hail and message:match("^hail") then
        if has_mission then
            e.self:Say("Tell me when you wish to " .. eq.say_link(enter, false, enter) .. ".")
        else
            e.self:Say("If your group is prepared, say " .. eq.say_link(request, false, request) .. ".")
        end
    else
        return false
    end
    return true
end

return M
