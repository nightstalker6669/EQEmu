local tower = require("rof2_deathknell_tower")
function event_enter_zone(e) tower.enter(e) end
function event_timer(e) tower.tick(e) end
function event_click_door(e) return tower.door(e) end
