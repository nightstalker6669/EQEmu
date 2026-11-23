local shrine = require("rof2_por_tunare_shrine")
function event_spawn(e) eq.set_timer("tunare_guardian", 400) end
function event_damage_taken(e) return shrine.guardian_damage(e) end
function event_timer(e) shrine.guardian_tick(e) end
