local ending = require("rof2_tss_vergalid_end")
function event_damage_taken(e) return ending.damage(e) end
function event_say(e) ending.say(e) end
