local scene=require('rof2_sod_kithicor_scene')
function event_encounter_load(e) scene.register(e.encounter) end
function event_encounter_unload(e) scene.unload() end
function event_timer(e) if e.timer=='kithicor_scene' then scene.tick() end end
