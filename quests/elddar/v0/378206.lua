local forest = require("rof2_por_tunare_forest")
function event_damage_taken(e) return forest.treant_damage(e) end
function event_timer(e) forest.treant_tick(e) end
