-- Original Mer mission route; compose existing callbacks.
local mer_previous_say=event_say
function event_say(e)
 if mer_previous_say then local r=mer_previous_say(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').say(e)
end

local mer_previous_trade=event_trade
function event_trade(e)
 if mer_previous_trade then local r=mer_previous_trade(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').trade(e)
end

local mer_previous_task_accepted=event_task_accepted
function event_task_accepted(e)
 if mer_previous_task_accepted then local r=mer_previous_task_accepted(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').accepted(e)
end
