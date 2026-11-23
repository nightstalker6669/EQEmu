local ending = require("rof2_tss_vergalid_end")
function event_death_zone(e) ending.death(e) end
function event_loot_zone(e) return ending.loot(e) end
