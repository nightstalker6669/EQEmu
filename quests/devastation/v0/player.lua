local maelin = require("rof2_por_maelin")

function event_loot(e) maelin.note_loot(e) end

function event_timer(e) maelin.loot_timer(e) end
