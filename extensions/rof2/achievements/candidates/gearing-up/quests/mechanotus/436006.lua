-- Exact replacement candidate for the reviewed Oculus-only Tabazin adapter.
-- Installer must verify predecessor hash and no external physical trade owner.
-- Refuse composition with an unknown physical callback rather than double-consume.
assert(event_trade==nil,'Tabazin has an unreviewed physical hand-in owner')
local oculus=require('achievement_quest_oculus')
local policy=require('achievement_quest_gearing_up_policy')
local gearing=require('achievement_quest_gearing_up').new({
 admit=function(c)return policy.enabled==true and policy.admit(c)end
})
local delivery=require('achievement_native_delivery_handin')
local previous_say=event_say
function event_say(e)
 local old
 if previous_say then old=previous_say(e)end
 if not gearing.say(e)then oculus.say(e)end
 return old
end
local previous_accepted=event_task_accepted
function event_task_accepted(e)
 local old
 if previous_accepted then old=previous_accepted(e)end
 if not gearing.accepted(e)then oculus.accepted(e)end
 return old
end
function event_trade(e)
 -- Both Gearing Up and Screw Loose are native task deliveries at this giver.
 -- No scripted escort/item reward or second consumer belongs to Tabazin.
 return delivery.trade(e)
end
