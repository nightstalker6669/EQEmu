-- Local zone hooks only. Other tasks pass through untouched; native global
-- player dispatch remains separate and is never called or replaced here.
local phase = require("rof2_underquarry_scent")
function event_enter_zone(e) phase.enter(e) end
function event_task_before_update(e) return phase.before_update(e) end
function event_task_update(e) phase.update(e) end
