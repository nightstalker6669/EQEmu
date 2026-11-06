-- Disabled source entry for the original world requester, not the instance actor.
local M = {}
local binding = require("rof2_arthicrex_task_bindings")

function M.say(e)
    if not binding.enabled or not binding.mission_controller_ready
        or binding.clearance_status ~= "COORDINATOR_BOUND_FROM_CLEAR_CONTRACT"
        or e.self:GetNPCTypeID() ~= binding.world_grogald then return end
    local message = e.message:lower()
    local task = binding.tasks.sporali
    if message:match("%f[%a]willing%f[%A]") then
        -- Original requester prerequisites, never an invented all-party gate.
        for _, key in ipairs({"abandon", "fallen", "desperate"}) do
            if not e.other:IsTaskCompleted(binding.tasks[key]) then return end
        end
        eq.task_selector({task})
    elseif message:match("%f[%a]way%f[%A]") then
        if not e.other:IsTaskActive(task) then return end
        local instance = e.other:GetSharedTaskInstanceID(task, binding.mission_zone, binding.mission_version)
        if not instance or instance == 0 or not binding.entry then return end
        local entry = binding.entry
        e.other:MovePCInstance(binding.mission_zone, instance, entry.x, entry.y, entry.z, entry.h)
        -- Native shared-task template creates the DZ. Never create a second one
        -- or fall back to some other cached zone/instance when admission is pending.
    end
end

return M
