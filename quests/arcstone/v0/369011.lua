local por = require("rof2_por_progression")

local function finish_access(client)
    if not client:IsTaskCompleted(920337) then return end
    client:GrantAlternateAdvancementAbility(574, 1, true)
end

function event_say(e)
    if not por.public(e.self, 369, 369011, 103459) then return end
    local words = por.words(e)
    if words == "hail" then
        if e.other:IsTaskCompleted(920337) then
            finish_access(e.other)
        else
            e.self:Say("I am looking for a way to enter the Plane of Music. Seek the Spirit of Ao.")
        end
    elseif words == "theater of blood" then
        finish_access(e.other)
    end
end

function event_trade(e)
    local client = e.other
    if por.public(e.self, 369, 369011, 103459) then
        if client:IsTaskCompleted(920309) and client:IsTaskCompleted(920306) and
            client:IsTaskCompleted(920336) and not client:IsTaskCompleted(920337) and por.offers(e,84162) then
            if not client:IsTaskActive(920337) then client:AssignTask(920337,e.self:GetID(),true) end
            if client:IsTaskActivityActive(920337,0) and por.take(e,{[84162]=1}) then
                por.advance(client,920337,0)
                finish_access(client)
            end
        end
    end
    e.self:ReturnHandinItems(client)
end
