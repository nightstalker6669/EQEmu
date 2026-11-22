local deathknell = require("rof2_por_deathknell")
local por = require("rof2_por_progression")

function event_click_door(e)
    if not por.current(380, 0, 0) or e.door:GetDoorDBID() ~= 16848 or e.door:GetDoorID() ~= 1 then return 0 end
    if not deathknell.move(e.self, "deathknell") then deathknell.request(e.self, "deathknell") end
    return 1
end
