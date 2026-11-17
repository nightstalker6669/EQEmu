local public=require('rof2_sod_rathe')
local scene=require('rof2_sod_rathe_scene')
function event_encounter_load(e)
 public.register()
 scene.register()
 eq.set_timer('rathe_scene',500,e.encounter)
end
function event_timer(e)
 if e.timer=='rathe_scene' then public.public_tick();scene.tick() end
end
function event_encounter_unload(e) scene.unload() end
