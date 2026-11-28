local delivery = require("achievement_native_delivery_handin")
local mission = require("hot_group_requester")

local function give_current_ritual_item(e)
    local item
    if e.other:IsTaskActivityActive(920400, 0) or e.other:IsTaskActivityActive(920400, 1) then
        item = 49187
    elseif e.other:IsTaskActivityActive(920401, 8) then
        item = 49195
    end
    if not item then return false end
    if e.other:CountItem(item) == 0 then
        e.other:SummonItem(item, 1)
    end
    if item == 49187 then
        e.self:Say("Collect the missing pages. Use the tattered journal when you have all four.")
    else
        e.self:Say("Read the ritual incantation in the mists of the northern waterfall.")
    end
    return true
end

function event_say(e)
    local text = e.message:lower()
    if mission.say(e, false) then
        return
    elseif text:find("pages", 1, true) and e.other:IsTaskCompleted(920131) then
        eq.task_selector({920400})
    elseif text:find("willing", 1, true) and e.other:IsTaskCompleted(920400) then
        eq.task_selector({920401})
    elseif text:match("%f[%a]help%f[%A]") then
        eq.task_selector({920131})
    elseif text:find("hail", 1, true) then
        if give_current_ritual_item(e) then
            return
        elseif e.other:IsTaskActive(920380) or e.other:IsTaskCompleted(920401) then
            mission.say(e, true)
        elseif e.other:IsTaskCompleted(920400) then
            e.self:Say("The grimoire may let us lay the expedition to rest. Are you [willing] to help?")
        elseif e.other:IsTaskCompleted(920131) then
            e.self:Say("The journal is missing several [pages]. Can you recover them?")
        else
            e.self:Say("Our expedition is missing. Will you [help] me find the crew?")
        end
    end
end

function event_trade(e)
    -- Native Deliver and the existing common reward ledger own consumption
    -- and payout. Reuse the deployed task-only return helper unchanged.
    delivery.trade(e)
end
