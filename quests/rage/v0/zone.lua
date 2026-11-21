local tower = require("rof2_por_razorthorn")
local trophies = require("rof2_por_sverag_trophies")

function event_death_zone(e)
    tower.sverag_death(e)
    trophies.bolvirk_death(e)
end
