-- BEGIN ROF2 XORBB EVIL-EYE CORPSE LENSES
-- Native parser dispatches local and global NPC callbacks independently.
-- Restrict these additions to the configured ordinary evil-eye types.
local xorbb_previous_killed_merit=event_killed_merit
function event_killed_merit(e)
 local result
 if xorbb_previous_killed_merit then result=xorbb_previous_killed_merit(e)end
 require('achievement_quest_xorbb_prison_loot').merit(e)
 return result
end
local xorbb_previous_death_complete=event_death_complete
function event_death_complete(e)
 local result
 if xorbb_previous_death_complete then result=xorbb_previous_death_complete(e)end
 require('achievement_quest_xorbb_prison_loot').death(e)
 return result
end
-- END ROF2 XORBB EVIL-EYE CORPSE LENSES
