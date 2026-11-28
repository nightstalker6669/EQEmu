local prerequisite = require("hot_public_prerequisites")
local routes = require("hot_alkabor_public_routes")

function event_say(e)
    if not prerequisite.say(e) then
        routes.say(e)
    end
end
