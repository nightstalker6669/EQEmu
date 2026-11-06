-- Standalone only at a verified empty NPC script slot; never overwrite another quest.
local mercenary=require('achievement_quest_xorbb_mercenary')
local previous_say=event_say
function event_say(e)
 local old;if previous_say then old=previous_say(e);if old and old~=0 then return old end end
 mercenary.say(e);return old
end
