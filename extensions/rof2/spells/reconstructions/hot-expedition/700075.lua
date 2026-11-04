local delivery = require("achievement_native_delivery_handin")

function event_say(e)
    local text = e.message:lower()
    if text:match("%f[%a]help%f[%A]") then
        eq.task_selector({920131})
    elseif text:find("hail", 1, true) then
        e.self:Say("Our expedition is missing. Will you [help] me find the crew?")
    end
end

function event_trade(e)
    -- Native Deliver and the existing common reward ledger own consumption
    -- and payout. Reuse the deployed task-only return helper unchanged.
    delivery.trade(e)
end
