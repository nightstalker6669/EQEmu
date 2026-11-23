local public = require("rof2_tss_ashengate_public")

function event_enter_zone(e) public.enter(e) end
function event_timer(e) public.tick(e) end
function event_combine_validate(e) return public.combine(e) end
