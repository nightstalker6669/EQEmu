-- Append once to the reviewed global/global_player.lua, preserving all earlier vetoes.
local centaur_previous_before_update=event_task_before_update
function event_task_before_update(e)
 if centaur_previous_before_update then
  local result=centaur_previous_before_update(e)
  if result and result~=0 then return result end
 end
 return require('achievement_quest_mesa_centaur').before_update(e)
end
