-- Existing separate global player callbacks remain untouched.
local function enabled()
 local v=tostring(eq.get_rule('SodProgression:EnableFieldOfScaleSlice')):lower()
 return (v=='true' or v=='1') and eq.get_zone():GetInstanceID()~=0
end
function event_enter_zone(e)
 if enabled() then require('rof2_sod_fieldofscale_scene').enter(e.self)
 else require('rof2_sod_fieldofscale_public').enter(e.self) end
end
function event_item_click_client(e)
 if enabled() then require('rof2_sod_fieldofscale_scene').crystal_click(e.self) end
end
function event_timer(e)
 if e.timer=='fos_public_receipts' then require('rof2_sod_fieldofscale_public').refresh(e.self) end
end
