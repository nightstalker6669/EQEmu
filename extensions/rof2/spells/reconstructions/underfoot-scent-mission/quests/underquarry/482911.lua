-- Draft source; install only as the complete coordinated mission.
local phase = require("rof2_underquarry_scent")
function event_spawn(e) phase.spawn(e) end
function event_death(e) return phase.allow_death(e) end
function event_death_complete(e) phase.death(e) end
function event_timer(e) phase.timer(e) end
