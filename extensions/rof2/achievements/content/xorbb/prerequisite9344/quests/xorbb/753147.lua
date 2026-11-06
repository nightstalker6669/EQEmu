-- Native task Deliver and the core return path own item/coin custody.
-- Installer must refuse an unknown physical hand-in owner or compose it explicitly.
local old_say=event_say
function event_say(e)
 local result
 if old_say then result=old_say(e)end
 require('achievement_quest_xorbb_npcs').say(e)
 return result
end
local old_combat=event_combat
function event_combat(e)
 local result
 if old_combat then result=old_combat(e)end
 require('achievement_quest_xorbb_npcs').combat(e)
 return result
end
local old_timer=event_timer
function event_timer(e)
 if require('achievement_quest_xorbb_npcs').timer(e)then return end
 if old_timer then return old_timer(e)end
end
