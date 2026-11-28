local prerequisite = require("hot_public_prerequisites")
local mission = require("hot_group_requester")

function event_say(e)
    if not prerequisite.say(e) then
        mission.say(e, true)
    end
end
