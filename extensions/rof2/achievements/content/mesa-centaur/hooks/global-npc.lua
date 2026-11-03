local centaur_previous_npc_spawn=event_spawn
function event_spawn(e)
 if centaur_previous_npc_spawn then
  local result=centaur_previous_npc_spawn(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_mesa_corruption').actor_spawn(e)
end
