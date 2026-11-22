local daosheen = require("rof2_por_daosheen")

function event_enter_zone(e) daosheen.enter(e) end
function event_click_door(e) return daosheen.door(e) end
function event_timer(e)
    if e.timer == "por_daosheen_settle" then daosheen.settle() end
end
