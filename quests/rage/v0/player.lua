local por = require("rof2_por_progression")
local maelin = require("rof2_por_maelin")

function event_enter_zone(e)
    eq.set_timer("por_vessel",1000)
end

function event_timer(e)
    if maelin.loot_timer(e) then return end
    if e.timer=="por_vessel" then maelin.vessel_tick(e.self) end
end

function event_click_door(e)
    if e.door:GetDoorDBID()==16807 and e.door:GetDoorID()==2 and
        por.enter(e.self,920306,375,10,354,63,3,0) then return 1 end
    return 0
end

function event_loot(e) maelin.note_loot(e) end
