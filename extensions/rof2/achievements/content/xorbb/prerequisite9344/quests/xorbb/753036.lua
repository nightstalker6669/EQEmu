-- Compose only with a reviewed predecessor; unknown callbacks need a merge.
local previous_say=event_say
function event_say(e)
 local old
 if previous_say then old=previous_say(e)end
 require('achievement_quest_xorbb').say(e)
 return old
end

local previous_accepted=event_task_accepted
function event_task_accepted(e)
 local old
 if previous_accepted then old=previous_accepted(e)end
 require('achievement_quest_xorbb').accepted(e)
 return old
end
