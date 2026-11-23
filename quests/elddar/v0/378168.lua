local forest = require("rof2_por_tunare_forest")
function event_say(e) forest.orgaf_say(e) end
function event_spawn(e) forest.orgaf_spawn(e) end
function event_timer(e) forest.orgaf_tick(e) end
function event_damage_taken(e) return forest.orgaf_damage(e) end
