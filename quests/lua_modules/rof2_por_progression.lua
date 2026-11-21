local M = {}

local function current(zone_id, version, instance)
    local zone = eq.get_zone()
    return zone:GetZoneID() == zone_id and zone:GetInstanceVersion() == version and
        (instance == nil or zone:GetInstanceID() == instance)
end
M.current = current

function M.public(npc, zone, npc_type, placement)
    return current(zone, 0, 0) and npc.valid and npc:GetNPCTypeID() == npc_type and
        npc:GetSpawnPointID() == placement
end

function M.words(event)
    return event.message:lower():match("^%s*(.-)%s*$"):gsub("[.!?]$", "")
end

function M.offer(npc, client, task, previous, level)
    if client.valid and client:GetLevel() >= level and
        (not previous or client:IsTaskCompleted(previous)) then
        return client:TaskSelectorFromNPC(npc, {task})
    end
    return false
end

function M.offers(event, item)
    for i=1,4 do
        local instance=event.trade["item"..i]
        if instance and instance.valid and instance:GetID()==item then return true end
    end
    return false
end

function M.take(event, items)
    return event.self:CheckHandin(event.other, {}, items, {})
end

function M.owned(client, task, zone_id, version)
    if not client or not client.valid or not client:IsTaskActive(task) then return end
    local instance = client:GetSharedTaskInstanceID(task, zone_id, version)
    if instance and instance > 0 then return instance end
end

function M.inside(client, task, zone, version)
    local instance = M.owned(client, task, zone, version)
    return instance and current(zone, version, instance)
end

function M.enter(client, task, zone, version, x, y, z, heading)
    local instance = M.owned(client, task, zone, version)
    if not instance then return false end
    client:MovePCInstance(zone, instance, x, y, z, heading)
    return true
end

function M.advance(client, task, activity, count)
    if client.valid and client:IsTaskActivityActive(task, activity) then
        client:UpdateTaskActivity(task, activity, count or 1)
        return true
    end
    return false
end

function M.player(mob)
    if not mob or not mob.valid then return end
    if not mob:IsClient() then mob = mob:GetUltimateOwner() end
    if mob and mob.valid and mob:IsClient() then return mob:CastToClient() end
end

return M
