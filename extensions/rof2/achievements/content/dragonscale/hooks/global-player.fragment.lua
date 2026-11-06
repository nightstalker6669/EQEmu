local dragonscale_previous_task_complete=event_task_complete
function event_task_complete(e)
 if dragonscale_previous_task_complete then local r=dragonscale_previous_task_complete(e);if r and r~=0 then return r end end
 return require('achievement_quest_dragonscale').complete(e)
end
local dragonscale_previous_task_before_update=event_task_before_update
function event_task_before_update(e)
 if dragonscale_previous_task_before_update then local r=dragonscale_previous_task_before_update(e);if r and r~=0 then return r end end
 return require('achievement_quest_dragonscale').before_update(e)
end

local dragonscale_previous_cast=event_cast
function event_cast(e)
 if dragonscale_previous_cast then local r=dragonscale_previous_cast(e);if r and r~=0 then return r end end
 return require('achievement_quest_dragonscale').banner(e)
end
