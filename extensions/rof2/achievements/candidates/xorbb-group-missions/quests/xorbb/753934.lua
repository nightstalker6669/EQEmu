-- Reserved Battle lord. Compose future camp/controller hooks before activation.
local Ranks=require('achievement_quest_xorbb_battle_ranks')
function event_killed_merit(e)Ranks.merit(e,'enemy_lord')end
function event_death_complete(e)Ranks.death(e,'enemy_lord')end
