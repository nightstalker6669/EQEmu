local scene=require('rof2_sod_scene')
function event_encounter_load(e)scene.register(e.encounter)end
function event_timer(e)if e.timer=='sod_scene'then scene.tick()end end
