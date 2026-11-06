-- Reserved Gems reward chest; verdict queue precedes the final native update.
local Chest=require('achievement_quest_xorbb_reward_chest')
function event_death_complete(e)Chest.death(e)end
