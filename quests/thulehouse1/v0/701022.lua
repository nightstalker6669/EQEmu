local mission = require("hot_group_requester")
local delivery = require("achievement_native_delivery_handin")

local potions = {
    { "potion of rot", 920402 },
    { "potion of the unknown", 920403 },
    { "potion of rage", 920404 },
    { "potion of horror", 920405 }
}

function event_say(e)
    local text = e.message:lower()
    for _, potion in ipairs(potions) do
        if text:find(potion[1], 1, true) then
            eq.task_selector({ potion[2] })
            return
        end
    end
    if text:match("^hail") then
        for _, potion in ipairs(potions) do
            if not e.other:IsTaskCompleted(potion[2]) and not e.other:IsTaskActive(920381) then
                e.self:Say("I need a [potion of rot], a [potion of the unknown], a [potion of rage], and a [potion of horror].")
                return
            end
        end
    end
    mission.say(e, true)
end

function event_trade(e)
    delivery.trade(e)
end
