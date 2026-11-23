local layout = require("rof2_tss_vergalid_layout")
local raid = require("rof2_tss_vergalid_raid")
local receipts = require("rof2_dodh_scene_receipts").read_one

local M = {}

local function activity_done(client, activity)
    local identity = client:GetSharedTaskIdentity()
    if not identity or identity == "" then return false end
    return not not receipts([[
        SELECT 1
        FROM shared_task_activity_state a
        JOIN shared_tasks t ON t.id = a.shared_task_id
        WHERE t.id = CAST(? AS UNSIGNED) AND t.task_id = 920361
          AND a.activity_id = ? AND a.completed_time IS NOT NULL AND a.done_count = 1
    ]], {identity, activity})
end

function M.click(e, binding)
    if not e.door.valid then return 0 end
    local client, door = e.self, e.door
    local number = door:GetDoorID()
    local uuid = raid.scope(client, binding)
    if not uuid then return 1 end

    local db_id = door:GetDoorDBID()
    if number == 1 and db_id == binding.exit_door then
        client:MovePC(403, binding.return_x, binding.return_y, binding.return_z, 0)
        return 1
    end
    local entities = eq.get_entity_list()
    if number == 41 or number == 42 then
        if not door:IsDoorOpen() then
            for _, part in ipairs({38, 39, 40}) do
                local leaf = entities:GetDoorsByDoorID(part)
                if leaf.valid then
                    if leaf:IsDoorOpen() then leaf:ForceClose(client) else leaf:ForceOpen(client) end
                end
            end
        end
        return 0
    end
    if binding == layout.ending and number == 4 and db_id == layout.ending.goru_gate then
        local status = client:GetVergalidEventLockout("Goru Uldrock")
        if status ~= "earned" and status ~= "inherited" then
            client:Message(13, "Goru still seals the shrine.")
            return 1
        end
    end
    if number == 18 then
        if binding ~= layout.ending or db_id ~= layout.ending.shrine_gate
            or door:GetDoorName() ~= "OBJ_VM_CRYPTDOORA" or door:GetOpenType() ~= 58 then return 1 end
        if not activity_done(client, 3) and client:GetVergalidEventLockout("Vergalid") ~= "earned" then
            client:Message(13, "The way into the depths is sealed.")
            return 1
        end
        local dx, dy = client:GetX() - door:GetX(), client:GetY() - door:GetY()
        if dx * dx + dy * dy > 45 * 45 or math.abs(client:GetZ() + 444.5) > 15 then return 1 end
        -- This type-58 portal carries the clicker across the original leaf.
        local instance_id = eq.get_zone():GetInstanceID()
        if dx > 0 then
            client:MovePCInstance(404, instance_id, -1777.140625, 881.990356, -444.584564, 128)
        else
            client:MovePCInstance(404, instance_id, -1715, 880, -444.367035, 384)
        end
        return 1
    end
    return 0
end

return M
