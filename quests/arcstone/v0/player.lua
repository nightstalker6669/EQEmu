local deathknell = require("rof2_por_deathknell")
local por=require("rof2_por_progression")
function event_click_door(e)
    if not por.current(369,0,0) then return 0 end
    if e.door:GetDoorDBID()==16732 and e.door:GetDoorID()==6 then
        e.self:MovePC(370,861,618,-265,0)
        return 1
    end
    if e.door:GetDoorDBID()==16734 and e.door:GetDoorID()==8 then
        if deathknell.move(e.self, "daosheen") then return 1 end
        for index=0,2 do
            if por.enter(e.self,920300+index,371,10+index,0,-95,2,0) then return 1 end
        end
    end
    return 0
end
