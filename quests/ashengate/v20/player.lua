local raid = require("rof2_tss_ashengate_raids")
local east = require("rof2_tss_ashengate_layout").east

function event_enter_zone(e) raid.enter(e, east) end
function event_timer(e) raid.tick(e, east) end
function event_click_door(e) return raid.door(e, east) end
