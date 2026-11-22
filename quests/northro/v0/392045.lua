local deathknell = require("rof2_por_deathknell_contacts")
local por = require("rof2_por_progression")

function event_say(e)
    if not por.public(e.self, 392, 392045, 105817) then return end
    if deathknell.suchun_request(e) then return end
    local client, words = e.other, por.words(e)
    if words == "ancient city" or (words == "hail" and not client:IsTaskCompleted(920307)) then
        por.offer(e.self, client, 920307, nil, 70)
    elseif words == "rest" or (words == "hail" and not client:IsTaskCompleted(920308)) then
        por.offer(e.self, client, 920308, 920307, 70)
    elseif words == "visit her" or words == "i will visit her" or words == "hail" then
        por.offer(e.self, client, 920309, 920308, 70)
    end
end
