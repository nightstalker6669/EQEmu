-- Append once to the reviewed global_npc script; never replace its existing body.
-- Installer must compare the complete predecessor hash and marker absence.
-- BEGIN ROF2 XORBB MERCENARY global_npc
local xorbb_bridge=require('achievement_quest_xorbb_mercenary_hooks')
local xorbb_previous_event_spawn=event_spawn
function event_spawn(e)
 local old
 if xorbb_previous_event_spawn then old=xorbb_previous_event_spawn(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.spawn(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_killed_merit=event_killed_merit
function event_killed_merit(e)
 local old
 if xorbb_previous_event_killed_merit then old=xorbb_previous_event_killed_merit(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.merit(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_timer=event_timer
function event_timer(e)
 local old
 if xorbb_previous_event_timer then old=xorbb_previous_event_timer(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.timer(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_death_complete=event_death_complete
function event_death_complete(e)
 local old
 if xorbb_previous_event_death_complete then old=xorbb_previous_event_death_complete(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.death(e)
 if result~=nil then return result end
 return old
end
-- END ROF2 XORBB MERCENARY global_npc
