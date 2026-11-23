local raid = require("rof2_tss_ashengate_raids")
local west = require("rof2_tss_ashengate_layout").west

function event_death_zone(e) raid.death(e, west) end
