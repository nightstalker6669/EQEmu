local forest = require("rof2_por_tunare_forest")
local rewards = require("rof2_por_tunare_rewards")
function event_death_zone(e)
    forest.death(e)
    rewards.death(e)
end
function event_loot_zone(e)
    if forest.loot(e) ~= 0 then return 1 end
    return rewards.loot(e)
end
