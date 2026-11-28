local ring = require("may_hunter_chelsith_ring")

function event_spawn(e) ring.spawn(e) end
function event_killed_merit(e) ring.killed_merit(e) end
function event_death_complete(e) ring.death_complete(e) end
