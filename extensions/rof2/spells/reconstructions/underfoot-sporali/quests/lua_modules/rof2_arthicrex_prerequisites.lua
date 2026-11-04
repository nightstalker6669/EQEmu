-- Disabled, unbound source: first two Mardan tasks only. No global callbacks.
-- Native task/actor IDs must be composed after coordinator Database clearance.
local M = {}
local binding = require("rof2_arthicrex_task_bindings")

local function enabled()
    return binding.enabled and binding.clearance_status == "COORDINATOR_BOUND_FROM_CLEAR_CONTRACT"
end

local function done(task, activity, count)
    -- Native quest context's initiator is e.other. Never use this helper to
    -- read a different member's state or manufacture completion from history.
    return eq.get_task_activity_done_count(task, activity) >= count
end

local speech = {
    mardan = {
        {task="abandon", activities={5}},
        {task="fallen", activities={10}, guards={{2,1},{6,1},{9,1}}},
    },
    entricalla = {{task="abandon", activities={2}}},
    haeger = {{task="fallen", activities={0,2}, guards_by_activity={[2]={{1,10}}}}},
    borgar = {{task="fallen", activities={3,6}, guards_by_activity={[6]={{4,5},{5,5}}}}},
    gronkin = {{task="fallen", activities={7,9}, guards_by_activity={[9]={{8,5}}}}},
}

local function permitted(client, previous)
    for _, key in ipairs(previous) do
        local id = binding.tasks[key]
        if not id or not client:IsTaskCompleted(id) then return false end
    end
    return true
end

function M.say(e, role)
    if not enabled() or not binding.npc_types[role]
        or e.self:GetNPCTypeID() ~= binding.npc_types[role] then return end
    local message = e.message:lower()
    if message:match("%f[%a]hail%f[%A]") then
        local pending = {}
        for _, row in ipairs(speech[role] or {}) do
            local task = binding.tasks[row.task]
            if task and e.other:IsTaskActive(task) then
                for _, activity in ipairs(row.activities) do
                    if e.other:IsTaskActivityActive(task, activity) then
                        local eligible = true
                        local guards = row.guards_by_activity and row.guards_by_activity[activity] or row.guards or {}
                        for _, guard in ipairs(guards) do
                            if not done(task, guard[1], guard[2]) then eligible = false end
                        end
                        if eligible then pending[#pending+1] = {task, activity} end
                        -- One pre-event stage per task, including a refusal.
                        break
                    end
                end
            end
        end
        for _, update in ipairs(pending) do
            e.other:UpdateTaskActivity(update[1], update[2], 1)
        end
        return
    end
    if role ~= "mardan" then return end
    local offers = {
        {key="abandon", phrase="patrol", previous={}},
        {key="fallen", phrase="support", previous={"abandon"}},
    }
    for _, offer in ipairs(offers) do
        if message:match("%f[%a]"..offer.phrase.."%f[%A]") then
            local task = binding.tasks[offer.key]
            if task and permitted(e.other, offer.previous) then eq.task_selector({task}) end
            return
        end
    end
    -- Do NOT offer/assign Desperate Deed or spawn its first wave here.
    -- Request acceptance is asynchronous. Its original world-wave ownership,
    -- actual accepted shared occurrence and failure lifecycle must be composed
    -- before that separate source controller can be enabled.
end

return M
