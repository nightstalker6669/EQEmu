local public = require("rof2_por_tunare_public")
local forest = require("rof2_por_tunare_forest")
function event_click_door(e) return public.enter(e) end
function event_enter_zone(e)
    forest.player_enter(e)
    e.self:ReconcileTunareGive()
end
function event_timer(e)
    forest.player_tick(e)
    public.follow_up(e)
end
