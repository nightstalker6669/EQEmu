-- New local hook; native dispatcher also preserves existing global enter callbacks.
function event_enter_zone(e)
  require('rof2_sod_scene').enter(e.self)
end
