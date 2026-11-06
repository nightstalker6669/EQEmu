local learner=require('achievement_god_learner')
local previous=event_say
function event_say(e)
 local old
 if previous then old=previous(e)end
 learner.say(e)
 return old
end
