local takish=require("rof2_por_takish")
function event_enter_zone(e) eq.set_timer("por_prince",1000) end
function event_timer(e)
    if e.timer=="por_prince" then takish.prince_tick(e.self) end
end
function event_click_door(e) return takish.private_exit(e,920308) end
