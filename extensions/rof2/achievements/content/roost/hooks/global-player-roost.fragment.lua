-- Roost achievement routes: preserve existing task callbacks.
local roost_previous_task_accepted=event_task_accepted
function event_task_accepted(e)
 if roost_previous_task_accepted then
  local result=roost_previous_task_accepted(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_roost').accepted(e)
end
local roost_previous_task_complete=event_task_complete
function event_task_complete(e)
 if roost_previous_task_complete then
  local result=roost_previous_task_complete(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_roost').complete(e)
end
local roost_previous_task_stage_complete=event_task_stage_complete
function event_task_stage_complete(e)
 if roost_previous_task_stage_complete then
  local result=roost_previous_task_stage_complete(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_roost').resume(e)
end
local roost_previous_enter_zone=event_enter_zone
function event_enter_zone(e)
 if roost_previous_enter_zone then
  local result=roost_previous_enter_zone(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_roost').resume(e)
end
local roost_previous_task_fail=event_task_fail
function event_task_fail(e)
 if roost_previous_task_fail then
  local result=roost_previous_task_fail(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_roost').resume(e)
end
local roost_previous_timer=event_timer
function event_timer(e)
 if require('achievement_quest_roost').timer(e)then return end
 if roost_previous_timer then return roost_previous_timer(e)end
end
local roost_previous_click_door=event_click_door
function event_click_door(e)
 local result=require('achievement_quest_roost').click_door(e)
 if result and result~=0 then return result end
 if roost_previous_click_door then return roost_previous_click_door(e)end
 return 0
end
local roost_previous_task_before_update=event_task_before_update
function event_task_before_update(e)
 local result=require('achievement_quest_roost').before_update(e)
 if result and result~=0 then return result end
 if roost_previous_task_before_update then return roost_previous_task_before_update(e)end
 return 0
end
