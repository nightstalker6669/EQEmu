local shrine = require("rof2_por_tunare_shrine")
function event_enter_zone(e) shrine.enter(e) end
function event_timer(e) shrine.tick(e) end
function event_click_door(e) return shrine.click(e) end
