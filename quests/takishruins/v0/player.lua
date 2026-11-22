local deathknell = require("rof2_por_deathknell")
local takish=require("rof2_por_takish")
function event_click_door(e)
    if e.door:GetDoorDBID() == 16844 and e.door:GetDoorID() == 1
        and deathknell.move(e.self, "suchun") then return 1 end
    return takish.used_public_tree(e)
end
