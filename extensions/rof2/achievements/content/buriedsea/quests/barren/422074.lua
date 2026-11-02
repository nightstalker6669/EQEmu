-- Native task delivery consumes the package; the controller starts the ambush
-- only after CheckHandin confirms custody. Unrelated goods and coins return.
local scenes=require('buriedsea_scene_native')
function event_trade(e) return scenes.recipient_trade(e) end
