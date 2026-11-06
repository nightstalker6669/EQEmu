local M=require('achievement_quest_xorbb_cooks')
local old_merit=event_killed_merit
function event_killed_merit(e)
 local result
 if old_merit then result=old_merit(e)end
 M.merit(e)
 return result
end
local old_combat=event_combat
function event_combat(e)
 local result
 if old_combat then result=old_combat(e)end
 M.combat(e)
 return result
end
local old_timer=event_timer
function event_timer(e)
 if M.timer(e)then return end
 if old_timer then return old_timer(e)end
end
