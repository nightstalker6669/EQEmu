-- Reserved Battle reward chest; real native /open only, no free item grants.
local Chest=require('achievement_quest_xorbb_reward_chest')
function event_death_complete(e)Chest.death(e)end
