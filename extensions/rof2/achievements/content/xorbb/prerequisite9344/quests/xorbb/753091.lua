-- Native task Deliver and the core return path own item/coin custody.
-- Installer must refuse an unknown physical hand-in owner or compose it explicitly.
local old_say=event_say
function event_say(e)
 local result
 if old_say then result=old_say(e)end
 require('achievement_quest_xorbb_npcs').say(e)
 return result
end
