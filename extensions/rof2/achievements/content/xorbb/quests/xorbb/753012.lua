-- Standalone only at a verified empty NPC script slot; never overwrite another quest.
local mercenary=require('achievement_quest_xorbb_mercenary')
local previous_say=event_say
function event_say(e)
 local old;if previous_say then old=previous_say(e);if old and old~=0 then return old end end
 mercenary.say(e);return old
end
local previous_accepted=event_task_accepted
function event_task_accepted(e)
 local old;if previous_accepted then old=previous_accepted(e);if old and old~=0 then return old end end
 mercenary.accepted(e);return old
end
assert(event_trade==nil,'Unreviewed physical hand-in consumer; do not double-consume')
function event_trade(e)
 return require('achievement_native_delivery_handin').trade(e)
end
