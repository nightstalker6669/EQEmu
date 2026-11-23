local ending = require("rof2_tss_vergalid_end")
function event_enter_zone(e) ending.arrive(e) end
function event_timer(e) ending.tick(e) end
local doors = require("rof2_tss_vergalid_doors")
local binding = require("rof2_tss_vergalid_layout").ending
function event_click_door(e) return doors.click(e, binding) end
