function event_encounter_load(e) require('rof2_sod_fieldofscale_public').register(e.encounter) end
function event_timer(e) if e.timer=='fos_world' then require('rof2_sod_fieldofscale_world').tick() end end
