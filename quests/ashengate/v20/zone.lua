local raid = require("rof2_tss_ashengate_raids")
local east = require("rof2_tss_ashengate_layout").east

function event_death_zone(e) raid.death(e, east) end
