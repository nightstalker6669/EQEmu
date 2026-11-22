local deathknell = require("rof2_por_deathknell")

function event_enter_zone(e) deathknell.arrival(e.self, "deathknell", {380, 1699, 2294, 746, 152}) end
