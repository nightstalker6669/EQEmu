local chests = require('achievement_quest_buriedsea_chests')
function event_spawn(e) chests.spawn(e) end
function event_death(e) return chests.death(e) end
