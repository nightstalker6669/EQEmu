local waves = require("rof2_arthicrex_desperate")
function event_death_complete(e) waves.agent_death(e) end
function event_timer(e) waves.agent_timer(e) end
