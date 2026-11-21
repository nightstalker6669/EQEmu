local por = require("rof2_por_progression")
local M = {}

local armor = {
    {skin=88069, item=88093, slot=17},
    {skin=88072, item=88094, slot=18},
    {skin=88076, item=88095, slot=2},
    {skin=88077, item=88096, slot=7},
    {skin=88074, item=88097, slot=12},
}

local function maelin(npc)
    return por.public(npc, 202, 202125, 58619)
end

local function open_skin_tasks(npc, client)
    if not client:IsTaskCompleted(920303) then
        return por.offer(npc,client,920303,nil,1)
    elseif not client:IsTaskCompleted(920335) then
        return por.offer(npc,client,920335,920303,1)
    else
        return por.offer(npc,client,920304,920335,1)
    end
end

function M.say(event)
    if not maelin(event.self) then return false end
    local words, client = por.words(event), event.other
    if words == "saga skins" or words == "prepare my new skins" then
        open_skin_tasks(event.self, client)
    elseif words == "become the vessel" or words == "i am ready to become the vessel" then
        por.offer(event.self,client,920305,920304,70)
    elseif words == "enraged flesh charm" then
        if client:IsTaskCompleted(920305) and client:CountItem(88099) == 0 then client:SummonItem(88099) end
    else
        return false
    end
    return true
end

function M.trade(event)
    if not maelin(event.self) then return false end
    local client, handled = event.other, false
    for item=88069,88077 do
        local activity = (item-88069)*2+1
        if client:IsTaskActivityActive(920303,activity) and client:CountItem(item+9)==0 and
            por.take(event,{[item]=1}) then
            client:SummonItem(item)
            client:SummonItem(item+9)
            por.advance(client,920303,activity)
            handled = true
        end
    end
    if client:IsTaskCompleted(920303) and not client:IsTaskCompleted(920335) and
        not client:IsTaskActive(920335) and (por.offers(event,88087) or por.offers(event,88089)) then
        client:AssignTask(920335,event.self:GetID(),true)
    end
    for i,item in ipairs({88087,88089}) do
        if client:IsTaskActivityActive(920335,i-1) and por.take(event,{[item]=1}) then
            por.advance(client,920335,i-1)
            handled = true
        end
    end
    if handled and client:IsTaskCompleted(920335) and not client:IsTaskActive(920304) and
        not client:IsTaskCompleted(920304) then
        client:AssignTask(920304,event.self:GetID(),true)
    end
    for i,entry in ipairs(armor) do
        if client:IsTaskActivityActive(920304,i+5) and client:CountItem(entry.item)==0 and
            por.take(event,{[entry.skin]=1}) then
            client:SummonItem(entry.item)
            por.advance(client,920304,i+5)
            handled = true
        end
    end
    return handled
end

function M.mark_skin(client, item)
    if not item or item < 88069 or item > 88077 or client:IsTaskCompleted(920303) then return end
    if not client:IsTaskActive(920303) and client:CountItem(item) > 0 then
        client:AssignTask(920303)
    end
    for skin=88069,88077 do
        if client:CountItem(skin) > 0 then
            por.advance(client,920303,(skin-88069)*2)
        end
    end
end

function M.note_loot(event)
    if not event.item or not event.item.valid or event.self:IsTaskCompleted(920303) then return end
    local item=event.item:GetID()
    if item>=88069 and item<=88077 then
        event.self:SetEntityVariable("por-new-skin",tostring(item))
        eq.set_timer("por_skin_attached",100)
    end
end

function M.loot_timer(event)
    if event.timer~="por_skin_attached" then return false end
    eq.stop_timer("por_skin_attached")
    local item=tonumber(event.self:GetEntityVariable("por-new-skin"))
    event.self:DeleteEntityVariable("por-new-skin")
    M.mark_skin(event.self,item)
    return true
end

function M.vessel_tick(client)
    if not por.current(374,0,0) or not client.valid then return end
    local task, stage = 920305
    if client:IsTaskActivityActive(task,0) then stage=0
    elseif client:IsTaskActivityActive(task,2) then stage=2 end
    if stage == nil then client:DeleteEntityVariable("por-vessel-sitting"); return end
    local dx, dy = client:GetX(), client:GetY()-80
    if dx*dx+dy*dy>100 or math.abs(client:GetZ()-31.7274)>30 or not client:IsSitting() then
        client:DeleteEntityVariable("por-vessel-sitting")
        return
    end
    for _,entry in ipairs(armor) do
        if client:GetItemIDAt(entry.slot) ~= entry.item then
            client:DeleteEntityVariable("por-vessel-sitting")
            return
        end
    end
    local run = client:GetSharedTaskIdentity()
    local key = run .. ":" .. stage .. ":"
    local marker = client:GetEntityVariable("por-vessel-sitting")
    if marker:sub(1,#key) ~= key then
        client:SetEntityVariable("por-vessel-sitting",key..os.time())
    elseif os.time()-(tonumber(marker:sub(#key+1)) or os.time())>=300 then
        por.advance(client,task,stage)
    end
end

return M
