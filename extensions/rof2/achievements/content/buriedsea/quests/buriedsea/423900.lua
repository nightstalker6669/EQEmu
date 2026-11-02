local scenes=require('buriedsea_scene_native')
function event_timer(e) return scenes.npc_timer(e) end
function event_death(e) return scenes.npc_death(e) end
