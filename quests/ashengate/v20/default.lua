local raid = require("rof2_tss_ashengate_raids")
local east = require("rof2_tss_ashengate_layout").east

function event_damage_taken(e)
    local kind = e.self:GetNPCTypeID()
    if kind >= 406938 and kind <= 406955 and kind ~= 406948 and kind ~= 406953 then
        return raid.damage(e, east)
    end
    if e.self:GetName():find("^a_Scarlet_Legion_") then return raid.tag_diversion(e) end
    return 0
end

function event_say(e)
    if e.self:GetNPCTypeID() == 406938 then raid.say(e, east) end
end
