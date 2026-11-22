local deathknell = require("rof2_por_deathknell_contacts")
local por = require("rof2_por_progression")

function event_say(e)
    if not por.public(e.self, 372, 372000, 103726) then return end
    if deathknell.sullon_request(e) then return end
    local client, words = e.other, por.words(e)
    if words == "hail" or words == "i can find out" then
        if client:IsTaskCompleted(920305) and client:CountItem(88099) > 0 then
            por.offer(e.self, client, 920306, 920305, 70)
        else
            e.self:Say("Find Maelin and learn to enter Sullon Zek's tower.")
        end
    elseif (words == "need another" or words == "samples") and
        por.owned(client, 920306, 375, 10) and client:CountItem(84157) == 0 and client:CountItem(84158) == 0 then
        client:SummonItem(84157)
    end
end

function event_task_accepted(e)
    if por.public(e.self, 372, 372000, 103726) and e.task_id == 920306 and
        e.other:IsTaskActive(920306) and e.other:CountItem(84157) == 0 and e.other:CountItem(84158) == 0 then
        e.other:SummonItem(84157)
    end
end

