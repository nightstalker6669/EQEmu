local muses = require("rof2_por_theater_muses")
function event_combat(e) muses.combat(e) end
function event_hp(e) muses.hp(e) end
function event_timer(e) muses.timer(e) end
function event_spawn(e) muses.spawn(e) end
function event_say(e) muses.begin_performance(e) end
function event_damage_taken(e) return muses.damage(e) end
