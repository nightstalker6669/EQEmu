-- Reserved King reward chest. Its encounter terminal challenge hook remains
-- unimplemented; do not enable the mission with this incomplete adapter alone.
local Chest=require('achievement_quest_xorbb_reward_chest')
function event_death_complete(e)Chest.death(e)end
