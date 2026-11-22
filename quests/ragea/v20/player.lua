local sullon = require("rof2_por_sullon")
function event_enter_zone(e) sullon.enter(e) end
function event_click_door(e) return sullon.door(e) end
function event_timer(e) sullon.ward(e) end
