-- Reserved Battle powershard golem. Actual top-rank merit only, no timer awards.
local Ranks=require('achievement_quest_xorbb_battle_ranks')
function event_killed_merit(e)Ranks.merit(e,'powershard_golem')end
function event_death_complete(e)Ranks.death(e,'powershard_golem')end
