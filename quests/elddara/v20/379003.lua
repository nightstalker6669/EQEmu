local shrine = require("rof2_por_tunare_shrine")
function event_damage_taken(e) return shrine.shrine_damage(e) end
