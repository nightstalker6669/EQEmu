local fall = require("rof2_dodh_fall")

function event_enter_zone(e)
    fall.enter(e)
end

function event_timer(e)
    fall.tick(e)
end
