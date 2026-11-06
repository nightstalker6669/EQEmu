local mission = require("rof2_arthicrex_sporali")
function event_spawn(e) mission.spawn(e) end
function event_death(e) return mission.allow_death(e) end
function event_death_complete(e) mission.death(e) end
function event_say(e) mission.say(e) end
function event_trade(e) return mission.trade(e) end
