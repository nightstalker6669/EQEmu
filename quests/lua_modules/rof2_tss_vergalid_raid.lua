local route = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")

local M = {}

function M.scope(client, binding)
    if not client or not client.valid then return end
    local zone = eq.get_zone()
    if zone:GetZoneID() ~= 404 or zone:GetInstanceVersion() ~= binding.version or zone:GetInstanceID() <= 0 then return end
    local uuid = client:GetVergalidMissionUUID(binding.task)
    if uuid and uuid ~= "" then return uuid end
end

function M.active(client, binding)
    local uuid = M.scope(client, binding)
    if uuid and client:IsTaskActive(binding.task) and
        client:GetSharedTaskInstanceID(binding.task, 404, binding.version) == eq.get_zone():GetInstanceID() then
        return uuid
    end
end

function M.alive_members(binding, uuid, point, radius)
    local members = {}
    for client in eq.get_entity_list():GetClientList().entries do
        if client:GetHP() > 0 and
            (not point or client:CalculateDistance(point[1], point[2], point[3]) <= radius)
            and M.scope(client, binding) == uuid then
            members[#members + 1] = client
        end
    end
    return members
end

function M.member(client, state, binding)
    return state and M.scope(client, binding) == state.uuid
end

function M.shared_step(members, binding, activity)
    for _, client in ipairs(members) do
        if route.advance(client, binding.task, activity) then return true end
    end
    return false
end

function M.arrive(e, binding)
    local uuid = M.active(e.self, binding)
    if not uuid then return end
    route.advance(e.self, binding.task, 0)
    return uuid
end

function M.death_owner(e, state, binding)
    local client = route.player(e.other)
    if not M.member(client, state, binding) then return end
    return client
end

function M.cap_damage(e, percent)
    local floor = math.floor(e.self:GetMaxHP() * percent / 100)
    local allowed = e.self:GetHP() - floor
    if allowed <= 0 then return -1 end
    if e.damage > allowed then return allowed end
    return 0
end

function M.spawn(state, npc_type, point, target)
    local npc = runtime.spawn_owned(npc_type, point, state.uuid)
    if not npc then return end
    state.spawned[npc:GetID()] = npc_type
    if target and target.valid then npc:AddToHateList(target, 1) end
    return npc
end

function M.clear(state)
    for id, npc_type in pairs(state.spawned) do
        local npc = runtime.owned(state.uuid, id, npc_type)
        if npc then npc:Depop(false) end
    end
end

return M
