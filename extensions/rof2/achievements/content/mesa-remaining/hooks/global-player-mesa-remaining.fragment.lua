-- Mesa remaining batch: preserve existing housing and quest callbacks.
local mesa_remaining_before_accepted=event_task_accepted
function event_task_accepted(e)
 if mesa_remaining_before_accepted then mesa_remaining_before_accepted(e)end
 require('achievement_quest_mesa_love').accepted(e)
 require('achievement_quest_mesa_tudor').resume(e)
 require('achievement_quest_mesa_defense').resume(e)
 require('achievement_quest_mesa_observations').resume(e)
 require('achievement_quest_mesa_escort').resume(e)
end
local mesa_remaining_before_complete=event_task_complete
function event_task_complete(e)
 if mesa_remaining_before_complete then
  local result=mesa_remaining_before_complete(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_mesa_love').complete(e)
end
local mesa_remaining_before_enter=event_enter_zone
function event_enter_zone(e)
 if mesa_remaining_before_enter then mesa_remaining_before_enter(e)end
 require('achievement_quest_mesa_love').enter(e)
 require('achievement_quest_mesa_tudor').resume(e)
 require('achievement_quest_mesa_defense').resume(e)
 require('achievement_quest_mesa_observations').resume(e)
 require('achievement_quest_mesa_escort').resume(e)
end
local mesa_remaining_before_timer=event_timer
function event_timer(e)
 if mesa_remaining_before_timer then mesa_remaining_before_timer(e)end
 if require('achievement_quest_mesa_love').timer(e)then return end
 if require('achievement_quest_mesa_defense').client_timer(e)then return end
 if require('achievement_quest_mesa_observations').timer(e)then return end
 if require('achievement_quest_mesa_escort').client_timer(e)then return end
 require('achievement_quest_mesa_tudor').timer(e)
end

local mesa_remaining_before_stage=event_task_stage_complete
function event_task_stage_complete(e)
 if mesa_remaining_before_stage then mesa_remaining_before_stage(e)end
 require('achievement_quest_mesa_tudor').resume(e)
 require('achievement_quest_mesa_defense').resume(e)
 require('achievement_quest_mesa_observations').resume(e)
 require('achievement_quest_mesa_escort').resume(e)
end
local mesa_remaining_before_fail=event_task_fail
function event_task_fail(e)
 if mesa_remaining_before_fail then mesa_remaining_before_fail(e)end
 require('achievement_quest_mesa_defense').failed(e)
 require('achievement_quest_mesa_escort').failed(e)
end
local mesa_remaining_before_death=event_death_complete
function event_death_complete(e)
 if mesa_remaining_before_death then mesa_remaining_before_death(e)end
 require('achievement_quest_mesa_defense').player_death(e)
 require('achievement_quest_mesa_escort').player_death(e)
end
local mesa_remaining_before_task_update=event_task_before_update
function event_task_before_update(e)
 if mesa_remaining_before_task_update then
  local result=mesa_remaining_before_task_update(e)
  if result and result~=0 then return result end
 end
 local escort_result=require('achievement_quest_mesa_escort').before_update(e)
 if escort_result~=0 then return escort_result end
 return require('achievement_quest_mesa_crafting').before_update(e)
end
