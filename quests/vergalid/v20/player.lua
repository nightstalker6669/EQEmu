local raid = require("rof2_tss_vergalid_raid")
local binding = require("rof2_tss_vergalid_layout").kellak
function event_enter_zone(e) raid.arrive(e, binding) end
local doors = require("rof2_tss_vergalid_doors")
function event_click_door(e) return doors.click(e, binding) end
