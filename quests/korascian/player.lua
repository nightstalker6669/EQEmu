-- Native global callbacks remain separately chained by the quest parser.
function event_enter_zone(e) require('rof2_sod_rathe').enter(e.self) end
