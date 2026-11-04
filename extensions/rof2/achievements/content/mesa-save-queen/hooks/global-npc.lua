local queen_previous_npc_spawn=event_spawn
function event_spawn(e)
 if queen_previous_npc_spawn then local r=queen_previous_npc_spawn(e);if r and r~=0 then return r end end
 require('achievement_quest_mesa_queen').npc_spawn(e)
end
local queen_previous_npc_timer=event_timer
function event_timer(e)
 if require('achievement_quest_mesa_queen').npc_timer(e)then return end
 if queen_previous_npc_timer then return queen_previous_npc_timer(e)end
end
