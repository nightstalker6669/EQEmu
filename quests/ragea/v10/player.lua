local maelin=require("rof2_por_maelin")
local por=require("rof2_por_progression")
local tower=require("rof2_por_razorthorn")
function event_enter_zone(e) tower.arrive(e.self) end
function event_loot(e) maelin.note_loot(e) end
function event_timer(e) maelin.loot_timer(e) end
function event_click_door(e)
    if e.door:GetDoorID()==37 and por.inside(e.self,920306,375,10) then
        e.self:MovePC(374,0,87,31.7,0)
        return 1
    end
    return 0
end
