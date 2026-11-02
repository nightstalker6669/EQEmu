-- Required whole-batch Mer mission hooks; existing callbacks/vetoes retained.

local mer_previous_enter_zone=event_enter_zone
function event_enter_zone(e)
 if mer_previous_enter_zone then local r=mer_previous_enter_zone(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').resume(e)
end

local mer_previous_task_accepted=event_task_accepted
function event_task_accepted(e)
 if mer_previous_task_accepted then local r=mer_previous_task_accepted(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').resume(e)
end

local mer_previous_task_update=event_task_update
function event_task_update(e)
 if mer_previous_task_update then local r=mer_previous_task_update(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').resume(e)
end

local mer_previous_task_complete=event_task_complete
function event_task_complete(e)
 if mer_previous_task_complete then local r=mer_previous_task_complete(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').resume(e)
end

local mer_previous_task_fail=event_task_fail
function event_task_fail(e)
 if mer_previous_task_fail then local r=mer_previous_task_fail(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').resume(e)
end

local mer_previous_timer=event_timer
function event_timer(e)
 if require('buriedsea_mer_native').timer(e)then return end
 if mer_previous_timer then return mer_previous_timer(e)end
end

local mer_previous_before_update=event_task_before_update
function event_task_before_update(e)
 if mer_previous_before_update then local r=mer_previous_before_update(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').before_update(e)
end

local mer_previous_click_door=event_click_door
function event_click_door(e)
 if mer_previous_click_door then local r=mer_previous_click_door(e);if r and r~=0 then return r end end
 if require('buriedsea_mer_native').click_door(e)then return 1 end
end
