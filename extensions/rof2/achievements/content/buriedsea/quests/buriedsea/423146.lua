-- Preserve original giver behavior; native core owns ordinary task progress.
local buriedsea_previous_say=event_say
function event_say(e)
 if buriedsea_previous_say then local r=buriedsea_previous_say(e);if r and r~=0 then return r end end
 return require('achievement_quest_buriedsea').say(e)
end

local buriedsea_previous_trade=event_trade
function event_trade(e)
 if buriedsea_previous_trade then local r=buriedsea_previous_trade(e);if r and r~=0 then return r end end
 return require('achievement_quest_buriedsea').trade(e)
end

local buriedsea_previous_task_accepted=event_task_accepted
function event_task_accepted(e)
 if buriedsea_previous_task_accepted then local r=buriedsea_previous_task_accepted(e);if r and r~=0 then return r end end
 return require('achievement_quest_buriedsea').accepted(e)
end
