local shrine = require("rof2_por_tunare_shrine")
function event_spawn(e) eq.set_timer("tunare_nalderin", 400) end
function event_say(e) shrine.nalderin_say(e) end
function event_damage_taken(e) return shrine.nalderin_damage(e) end
function event_timer(e) shrine.nalderin_tick(e) end
