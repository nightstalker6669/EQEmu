-- Exact zone player preimage absent; separate globals remain untouched.
function event_click_door(e)
 return require('rof2_sod_fieldofscale').door(e)
end
function event_enter_zone(e)
 require('rof2_sod_fieldofscale_public').enter(e.self)
end
function event_timer(e)
 if e.timer=='fos_public_receipts' then require('rof2_sod_fieldofscale_public').refresh(e.self) end
end
