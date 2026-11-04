-- Draft source: compose with the complete owned mission before installation.
-- The native shared-task template creates the instance; never create a second
-- expedition or fall back to a cached same-zone dynamic zone.
local TASK = 920136
local prerequisites = {
    {920132, "Unrest in the Metal Mines"},
    {920133, "Chaos in the Stonequarry"},
    {920134, "An Alternative Explanation"},
    {920135, "Overheated Production"},
}

local function word(message, keyword)
    return message:match("%f[%a]" .. keyword .. "%f[%A]") ~= nil
end

function event_say(e)
    local message = e.message:lower()
    if word(message, "hail") then
        e.self:Say("Lady Envy's clockworks threaten the Underquarry. Will you [accept] the mission? If your party already has it, tell me when you wish to [go].")
    elseif word(message, "accept") then
        -- Original request prerequisite chain. Native admission owns the real
        -- shared-task membership, player limits, level and replay checks.
        -- Do not invent an all-group prerequisite or grant completion flags.
        for _, requirement in ipairs(prerequisites) do
            if not e.other:IsTaskCompleted(requirement[1]) then
                e.self:Say("First complete " .. requirement[2] .. " for Clontik.")
                return
            end
        end
        eq.task_selector({TASK})
    elseif word(message, "go") then
        if not e.other:IsTaskActive(TASK) then
            e.self:Say("You need to be a member of the mission before I can send you there.")
            return
        end
        local instance_id = e.other:GetSharedTaskInstanceID(TASK, 482, 1)
        if not instance_id or instance_id == 0 then
            e.self:Say("Your mission instance is not ready. Speak to me again shortly.")
            return
        end
        -- Source wiki candidate /loc Y=-15,X=145,Z=-192; heading is a
        -- labelled local default. Native floor/entry acceptance is pending.
        e.other:MovePCInstance(482, instance_id, 145, -15, -192, 0)
    end
end
