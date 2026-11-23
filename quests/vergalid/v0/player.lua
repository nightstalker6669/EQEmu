local levers = {[41] = 17399, [42] = 17400}
local leaves = {[38] = 17396, [39] = 17397, [40] = 17398}
local next_actuation = 0

function event_click_door(e)
    local zone = eq.get_zone()
    local door, actor = e.door, e.self
    if zone:GetZoneID() ~= 404 or zone:GetInstanceVersion() ~= 0 or zone:GetInstanceID() ~= 0
        or not door.valid or not actor.valid then return 0 end
    local packet, id = door:GetDoorID(), door:GetDoorDBID()
    if leaves[packet] == id then return 1 end
    if levers[packet] ~= id then return 0 end

    local now = os.time()
    if door:IsDoorOpen() or now < next_actuation then return 1 end
    local entities = eq.get_entity_list()
    local assembly = {}
    for _, part in ipairs({38, 39, 40}) do
        local leaf = entities:GetDoorsByDoorID(part)
        if not leaf.valid or leaf:GetDoorDBID() ~= leaves[part] or not leaf:GetDisableTimer() then
            return 1
        end
        assembly[#assembly + 1] = leaf
    end

    local raising = not assembly[1]:IsDoorOpen()
    for _, leaf in ipairs(assembly) do
        if raising and not leaf:IsDoorOpen() then
            leaf:ForceOpen(actor)
        elseif not raising and leaf:IsDoorOpen() then
            leaf:ForceClose(actor)
        end
    end
    next_actuation = now + 6
    -- The ordinary lever packet still performs the handle animation.
    return 0
end
