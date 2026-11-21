local takish = require("rof2_por_takish")

function event_enter_zone(e)
  local client = eq.get_entity_list():GetClientByID(e.self:GetID());
  
  if client.valid and not takish.stay_in_modern_ro(client,392) then
    client:MovePC(34, 923, 2649, -24, 484); -- Zone: nro
  end
end
