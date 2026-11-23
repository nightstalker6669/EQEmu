local shrine = require("rof2_por_tunare_shrine")
function event_say(e) shrine.brazlin_say(e) end
function event_damage_taken(e) return -1 end
