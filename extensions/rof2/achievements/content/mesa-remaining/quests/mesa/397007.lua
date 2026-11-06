local M=require('achievement_quest_mesa')
local R=require('achievement_quest_mesa_remaining')
function event_say(e)if not R.say(e)then M.say(e)end end
function event_trade(e)M.trade(e)end
