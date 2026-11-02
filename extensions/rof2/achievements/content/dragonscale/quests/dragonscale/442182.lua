local dragonscale=require('achievement_quest_dragonscale')
local dragonscale_previous_say=event_say
function event_say(e)
 if dragonscale_previous_say then local r=dragonscale_previous_say(e);if r and r~=0 then return r end end
 return dragonscale.rescue_hail(e)
end
