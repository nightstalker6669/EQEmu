local tower = require("rof2_deathknell_tower")
function event_damage_taken(e) return tower.damage(e) end
