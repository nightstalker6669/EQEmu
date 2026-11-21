local por=require("rof2_por_progression")
function event_click_door(e)
    if por.current(369,0,0) and e.door:GetDoorDBID()==16734 and e.door:GetDoorID()==8 then
        for index=0,2 do
            if por.enter(e.self,920300+index,371,10+index,0,-95,2,0) then return 1 end
        end
    end
    return 0
end
