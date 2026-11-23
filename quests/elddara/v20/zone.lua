local shrine = require("rof2_por_tunare_shrine")
function event_death_zone(e) shrine.death(e) end
function event_spawn_zone(e) shrine.spawn(e) end
function event_loot_zone(e) return shrine.loot(e) end
