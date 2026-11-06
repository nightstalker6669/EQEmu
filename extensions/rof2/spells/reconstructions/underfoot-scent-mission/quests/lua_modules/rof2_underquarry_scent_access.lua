-- Draft source. Existing native instance membership survives task completion;
-- an active shared-task pointer is deliberately not a chest-access condition.
local M = {}

function M.instance()
    local instance_id = eq.get_zone_instance_id()
    if not instance_id or instance_id == 0 then return nil end
    if eq.get_instance_zone_id_by_id(instance_id) ~= 482
        or eq.get_instance_version_by_id(instance_id) ~= 1 then
        return nil
    end
    if eq.get_instance_timer_by_id(instance_id) <= 0 then return nil end
    return instance_id
end

function M.member(client)
    if not client then return false end
    local instance_id = M.instance()
    if not instance_id then return false end
    if not eq.check_instance_by_char_id(instance_id, client:CharacterID()) then
        return false
    end
    return true, instance_id
end

return M
