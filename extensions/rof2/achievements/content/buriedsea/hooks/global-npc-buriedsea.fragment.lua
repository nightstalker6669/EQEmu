-- Compose with existing global NPC spawn behavior; never replace its veto.
local buriedsea_previous_npc_spawn=event_spawn
function event_spawn(e)
 if buriedsea_previous_npc_spawn then local r=buriedsea_previous_npc_spawn(e);if r and r~=0 then return r end end
 return require('buriedsea_totem_sources').spawn(e)
end

local buriedsea_previous_npc_death=event_death
function event_death(e)
 if buriedsea_previous_npc_death then local r=buriedsea_previous_npc_death(e);if r and r~=0 then return r end end
 if eq.get_zone_id()==429 then return require('buriedsea_mer_native').npc_death(e)end
 return 0
end
