local mesa_npc_before_spawn=event_spawn
function event_spawn(e)
 if mesa_npc_before_spawn then mesa_npc_before_spawn(e)end
 require('achievement_quest_mesa_resources').spawn(e)
end

local mesa_npc_before_enter=event_enter
function event_enter(e)
 if mesa_npc_before_enter then mesa_npc_before_enter(e)end
 require('achievement_quest_mesa_resources').enter(e)
end

local mesa_npc_before_killed_merit=event_killed_merit
function event_killed_merit(e)
 if mesa_npc_before_killed_merit then mesa_npc_before_killed_merit(e)end
 require('achievement_quest_mesa_resources').killed_merit(e)
end

local mesa_npc_before_timer=event_timer
function event_timer(e)
 if require('achievement_quest_mesa_wand').timer(e)then return end
 if mesa_npc_before_timer then mesa_npc_before_timer(e)end
 require('achievement_quest_mesa_scenes').npc_timer(e)
end
