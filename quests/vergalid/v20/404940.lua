local kellak = require("rof2_tss_kellak")
function event_spawn(e) kellak.spawn(e) end
function event_damage_taken(e) return kellak.damage(e) end
function event_timer(e) kellak.tick(e) end
