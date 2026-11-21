local takish = require("rof2_por_takish")

function event_enter_zone(e)
  local client = eq.get_entity_list():GetClientByID(e.self:GetID());
  
  if client.valid and not takish.stay_in_modern_ro(client,393) then
    client:MovePC(35, 1033, -1447, -25, 152); -- Zone: sro
  end
end
