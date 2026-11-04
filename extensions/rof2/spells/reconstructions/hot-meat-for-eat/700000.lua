-- Original Lerg task request; native task delivery owns item consumption.
local delivery = require("achievement_native_delivery_handin")

function event_say(e)
    local text = e.message:lower()
    if text:match("^%s*join%s*$") then
        eq.task_selector({920130})
    elseif text:find("hail", 1, true) then
        e.self:Say("I need help gathering gorilla meat. Would you like to [join] the hunt?")
    end
end

function event_trade(e)
    -- Existing shared native-delivery receipt handling; no second consumer,
    -- guessed refund, manual deletion, currency grant or achievement override.
    delivery.trade(e)
end
