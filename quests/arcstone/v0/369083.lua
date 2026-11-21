local por = require("rof2_por_progression")

function event_say(e)
    if not por.public(e.self, 369, 369083, 103665) then return end
    local client, words = e.other, por.words(e)
    if words == "hail" or words == "my group" then
        if not client:IsTaskCompleted(920300) then
            por.offer(e.self, client, 920300, nil, 70)
        elseif not client:IsTaskCompleted(920301) then
            por.offer(e.self, client, 920301, 920300, 70)
        elseif not client:IsTaskCompleted(920302) then
            por.offer(e.self, client, 920302, 920301, 70)
        elseif not client:IsTaskCompleted(920336) then
            por.offer(e.self, client, 920336, 920302, 70)
        end
    elseif words == "library" then
        por.offer(e.self, client, 920300, nil, 70)
    elseif words == "oubliette" then
        por.offer(e.self, client, 920301, 920300, 70)
    elseif words == "laboratory" then
        por.offer(e.self, client, 920302, 920301, 70)
    end
end

function event_task_accepted(e)
    if not por.public(e.self, 369, 369083, 103665) or e.task_id ~= 920302 then return end
    if e.other:IsTaskActive(920302) and e.other:CountItem(36145) == 0 then e.other:SummonItem(36145) end
end

function event_trade(e)
    if por.public(e.self,369,369083,103665) and e.other:IsTaskCompleted(920302) and
        not e.other:IsTaskCompleted(920336) and por.offers(e,36144) then
        if not e.other:IsTaskActive(920336) then e.other:AssignTask(920336,e.self:GetID(),true) end
        if e.other:IsTaskActivityActive(920336,0) and por.take(e,{[36144]=1}) then
            por.advance(e.other,920336,0)
        end
    end
    e.self:ReturnHandinItems(e.other)
end
