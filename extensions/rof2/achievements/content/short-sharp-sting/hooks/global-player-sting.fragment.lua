local sting_accept_before=event_task_accepted
function event_task_accepted(e)
 if sting_accept_before then sting_accept_before(e)end
 require('achievement_quest_sting').accepted(e)
end
local sting_update_before=event_task_update
function event_task_update(e)
 if sting_update_before then sting_update_before(e)end
 require('achievement_quest_sting').updated(e)
end
local sting_enter_before=event_enter_zone
function event_enter_zone(e)
 if sting_enter_before then sting_enter_before(e)end
 require('achievement_quest_sting').enter(e)
end
local sting_timer_before=event_timer
function event_timer(e)
 if require('achievement_quest_sting').timer(e)then return end
 if sting_timer_before then return sting_timer_before(e)end
end
