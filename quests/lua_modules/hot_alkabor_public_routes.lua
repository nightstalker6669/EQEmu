local zone_id = 709
local routes = {
    { npc = 709002, spawn = 137743, name = "Rivervale", arrival = { 1035.375, 48.25, -30.1699, 0 } },
    { npc = 709036, spawn = 137826, name = "Everfrost", arrival = { -1482.875, 308.625, -208.8108, 0 } },
    { npc = 709076, spawn = 137926, name = "Faydark", arrival = { -4519.5, -339.5, -352.2482, 0 } },
    { npc = 709089, spawn = 137953, name = "Dreadlands", arrival = { -6243, -574, -670.1550, 0 } }
}

local M = {}

function M.say(e)
    if not e.other.valid then
        return
    end
    local zone = eq.get_zone()
    if zone:GetZoneID() ~= zone_id or zone:GetInstanceVersion() ~= 0 or zone:GetInstanceID() ~= 0 or
        not e.other:HasCompletedAchievement(1800000) or not e.other:CanEnterZone("alkabormare", 0) then
        return
    end
    if math.abs(e.self:GetZ() - e.other:GetZ()) > 12 then
        return
    end
    local message = e.message:lower()
    local current
    for i, route in ipairs(routes) do
        if e.self:GetNPCTypeID() == route.npc and e.self:GetSpawnPointID() == route.spawn then
            current = i
            break
        end
    end
    if not current then
        return
    end

    for _, neighbor in ipairs({ current - 1, current + 1 }) do
        local target = routes[neighbor]
        if target then
            local action = "cross to " .. target.name
            if message == action:lower() then
                e.other:MovePC(zone_id, unpack(target.arrival))
                return
            elseif message:match("^hail") then
                e.self:Say("I can send you across the gap. Say " .. eq.say_link(action, false, action) .. ".")
            end
        end
    end
end

return M
