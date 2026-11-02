-- Original Mer mission route; compose existing callbacks.
local mer_previous_waypoint_arrive=event_waypoint_arrive
function event_waypoint_arrive(e)
 if mer_previous_waypoint_arrive then local r=mer_previous_waypoint_arrive(e);if r and r~=0 then return r end end
 return require('buriedsea_mer_native').waypoint(e)
end
