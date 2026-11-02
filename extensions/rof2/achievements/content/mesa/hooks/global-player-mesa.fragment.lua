local mesa_before_task_accepted=event_task_accepted
function event_task_accepted(e)
 if mesa_before_task_accepted then mesa_before_task_accepted(e)end
 require('achievement_quest_mesa').accepted(e)
end

local mesa_before_task_complete=event_task_complete
function event_task_complete(e)
 if mesa_before_task_complete then mesa_before_task_complete(e)end
 require('achievement_quest_mesa').complete(e)
end

local mesa_before_task_fail=event_task_fail
function event_task_fail(e)
 if mesa_before_task_fail then mesa_before_task_fail(e)end
 require('achievement_quest_mesa').failed(e)
end

local mesa_before_enter_zone=event_enter_zone
function event_enter_zone(e)
 if mesa_before_enter_zone then mesa_before_enter_zone(e)end
 require('achievement_quest_mesa').enter(e)
end

local mesa_before_death_complete=event_death_complete
function event_death_complete(e)
 if mesa_before_death_complete then mesa_before_death_complete(e)end
 require('achievement_quest_mesa').death(e)
end

local mesa_before_timer=event_timer
function event_timer(e)
 if mesa_before_timer then mesa_before_timer(e)end
 require('achievement_quest_mesa').timer(e)
end

local mesa_before_click=event_click_door
function event_click_door(e)
 local r;if mesa_before_click then r=mesa_before_click(e)end
 if r~=nil and r~=0 then return r end
 return require('achievement_quest_mesa').click_door(e)
end

local mesa_before_object_click=event_click_object
function event_click_object(e)
 local r;if mesa_before_object_click then r=mesa_before_object_click(e)end
 if r~=nil and r~=0 then return r end
 return require('achievement_quest_mesa').click_object(e)
end
