function event_enter_zone(e)
  require('rof2_sod_kithicor_scene').enter(e.self)
end
function event_click_door(e)
  require('rof2_sod_kithicor').book(e)
end
function event_timer(e)
  require('rof2_sod_kithicor').timer(e)
end
