local raid = require("rof2_tss_ashengate_raids")
local west = require("rof2_tss_ashengate_layout").west

function event_damage_taken(e)
    local kind = e.self:GetNPCTypeID()
    if kind >= 406956 and kind <= 406978 and kind ~= 406961 and kind ~= 406972 then
        return raid.damage(e, west)
    end
    return 0
end

function event_say(e)
    if e.self:GetNPCTypeID() == 406973 then raid.say(e, west) end
end
