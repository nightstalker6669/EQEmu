local takish=require("rof2_por_takish")
function event_enter_zone(e) takish.arrive(e.self,920307) end
function event_click_door(e) return takish.private_exit(e,920307) end
