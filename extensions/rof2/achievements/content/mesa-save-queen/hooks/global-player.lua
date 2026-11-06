-- Compose only the hooks needed by physical quest supply and native SpeakWith.
local queen_previous_enter=event_enter_zone
function event_enter_zone(e)
 if queen_previous_enter then queen_previous_enter(e)end
 require('achievement_quest_mesa_queen').resume(e)
end
local queen_previous_accepted=event_task_accepted
function event_task_accepted(e)
 if queen_previous_accepted then queen_previous_accepted(e)end
 require('achievement_quest_mesa_queen').accepted(e)
end
local queen_previous_complete=event_task_complete
function event_task_complete(e)
 if queen_previous_complete then queen_previous_complete(e)end
 require('achievement_quest_mesa_queen').resume(e)
end
local queen_previous_failed=event_task_fail
function event_task_fail(e)
 if queen_previous_failed then queen_previous_failed(e)end
 require('achievement_quest_mesa_queen').resume(e)
end
local queen_previous_timer=event_timer
function event_timer(e)
 if require('achievement_quest_mesa_queen').timer(e)then return end
 if queen_previous_timer then return queen_previous_timer(e)end
end
local queen_previous_before=event_task_before_update
function event_task_before_update(e)
 if queen_previous_before then local r=queen_previous_before(e);if r and r~=0 then return r end end
 return require('achievement_quest_mesa_queen').before_update(e)
end
