local sky = require("rof2_por_skylance")

function event_enter_zone(e) sky.arrive(e.self,10) end
function event_click_door(e) return sky.door(e,10) end

function event_timer(e) sky.return_timer(e,10) end
function event_enter_area(e) sky.return_area(e,10) end
