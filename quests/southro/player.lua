local takish = require("rof2_por_takish")

local function from_innothule(client)
    local zone = eq.get_zone()
    if zone:GetZoneID() ~= 393 or zone:GetInstanceVersion() ~= 0 or zone:GetInstanceID() ~= 0 then
        return false
    end

    local key = "por_tunare:innothule_south_ro:" .. client:CharacterID()
    if eq.get_data(key) ~= "1" then return false end
    eq.delete_data(key)

    local x, y, z = client:GetX() + 342, client:GetY() + 596, client:GetZ() - 146
    return x*x + y*y <= 8*8 and math.abs(z) <= 20
end

function event_enter_zone(e)
    local client = eq.get_entity_list():GetClientByID(e.self:GetID())
    if client.valid and not from_innothule(client) and not takish.stay_in_modern_ro(client, 393) then
        client:MovePC(35, 1033, -1447, -25, 152)
    end
end
