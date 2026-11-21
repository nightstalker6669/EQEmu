local sky = require("rof2_por_skylance")

function event_enter_zone(e)
    sky.arrive(e.self,12)
    eq.set_timer("por_scene_replay",3000)
end
function event_timer(e)
    if e.timer=="por_scene_replay" then sky.replay(e.self,12) end
    sky.return_timer(e,12)
end
function event_click_door(e) return sky.door(e,12) end
function event_enter_area(e) sky.return_area(e,12) end
