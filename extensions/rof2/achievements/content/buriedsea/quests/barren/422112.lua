-- Original Mer mission route; compose existing callbacks.
local mer_previous_trade=event_trade
function event_trade(e)
 if mer_previous_trade then local r=mer_previous_trade(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').trade(e)
end
