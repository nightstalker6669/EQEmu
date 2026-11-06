-- One note per qualifying native kill-merit participant, placed on the real corpse.
-- An unknown predecessor with its own note producer requires an explicit merge.
local old_merit=event_killed_merit
function event_killed_merit(e)
 local result
 if old_merit then result=old_merit(e)end
 require('achievement_quest_xorbb_loot').merit(e)
 return result
end
local old_complete=event_death_complete
function event_death_complete(e)
 local result
 if old_complete then result=old_complete(e)end
 require('achievement_quest_xorbb_loot').death(e)
 return result
end
