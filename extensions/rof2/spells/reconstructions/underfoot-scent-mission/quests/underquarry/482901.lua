-- Draft source; install only as the complete coordinated mission.
local phase = require("rof2_underquarry_scent")
function event_spawn(e) phase.spawn(e) end
function event_death(e) return phase.allow_death(e) end
function event_death_complete(e) phase.death(e) end
function event_say(e) phase.say(e) end
function event_hate_list(e) phase.hate(e) end
function event_trade(e) phase.trade(e) end
