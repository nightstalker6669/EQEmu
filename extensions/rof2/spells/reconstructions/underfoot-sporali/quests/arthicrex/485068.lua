local tasks = require("rof2_arthicrex_prerequisites")
local waves = require("rof2_arthicrex_desperate")
function event_say(e) tasks.say(e, "mardan"); waves.say(e) end
function event_timer(e) waves.timer(e) end
