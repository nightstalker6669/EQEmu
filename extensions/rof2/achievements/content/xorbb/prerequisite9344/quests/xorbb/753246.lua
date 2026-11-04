local P=require('achievement_quest_xorbb_prison')
function event_spawn(e)P.spawn(e)end
function event_say(e)P.say(e)end
function event_combat(e)P.combat(e)end
function event_timer(e)P.timer(e)end
function event_killed_merit(e)require('achievement_quest_xorbb_spirit').merit(e)end
