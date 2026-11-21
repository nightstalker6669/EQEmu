local takish = require("rof2_por_takish")

-- Ro Portal
function event_spell_effect(e)
  local mob = e.target;
  if (mob.valid and mob:IsClient()) then 
    local client = mob:CastToClient();
    if (client.valid) then
      if takish.stay_in_modern_ro(client, eq.get_zone():GetZoneID()) then
        return 0 -- Use the native spell destination for this recipient.
      end
      client:MovePC(34, 820, 1374, 10, 80); -- Zone: nro
    end
  end

  return 1;
end

