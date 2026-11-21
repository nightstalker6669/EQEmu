local takish = require("rof2_por_takish")

-- Translocate: Ro 
function event_spell_effect_translocate_complete(e) 
  local client = eq.get_entity_list():GetClientByID(e.target:GetID());
  
  if (client.valid) then
    if takish.stay_in_modern_ro(client, eq.get_zone():GetZoneID()) then
      return 0 -- Use the native spell destination for this recipient.
    end
    client:MovePC(34, 820, 1374, 10, 80); -- Zone: nro
  end

  return 1;
end
