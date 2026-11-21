local por = require("rof2_por_progression")

function event_say(e)
    if not por.public(e.self, 392, 392088, 105896) then return end
    local client, words = e.other, por.words(e)
    if words == "hail" and client:IsTaskActivityActive(920308, 3) then
        por.advance(client, 920308, 3)
    elseif words == "hail" or words == "i am willing" or words == "i am prepared" then
        if por.owned(client, 920308, 376, 10) then
            por.advance(client, 920308, 0)
            por.enter(client, 920308, 376, 10, -983, 269, 62, 0)
        elseif por.owned(client, 920309, 376, 11) then
            if client:IsTaskActivityActive(920309, 0) and client:CountItem(84155) == 0 then client:SummonItem(84155) end
            por.advance(client, 920309, 0)
            if words == "i am prepared" and not client:IsTaskActivityActive(920309, 0) then
                por.enter(client, 920309, 376, 11, -983, 269, 62, 0)
            end
        end
    end
end

