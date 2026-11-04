local mission = require("rof2_arthicrex_sporali")
local waves = require("rof2_arthicrex_desperate")
function event_task_accepted(e) waves.accepted(e) end
function event_task_update(e) waves.updated(e) end
function event_task_before_update(e)
    if mission.before_update(e) ~= 0 then return 1 end
    return waves.before_update(e)
end
-- Native QuestParserCollection independently calls global/default handlers.
