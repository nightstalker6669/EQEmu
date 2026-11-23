local raid = require("rof2_tss_ashengate_raids")
local west = require("rof2_tss_ashengate_layout").west

function event_enter_zone(e) raid.enter(e, west) end
function event_timer(e) raid.tick(e, west) end
function event_click_door(e) return raid.door(e, west) end
