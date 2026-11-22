local suchun = require("rof2_por_suchun")
function event_damage_taken(e)
    if e.self:GetNPCTypeID() == 377013 and e.self:GetSpawnPointID() > 0 then return 0 end
    return suchun.damage(e)
end
function event_spawn(e) suchun.wall_spawn(e) end
