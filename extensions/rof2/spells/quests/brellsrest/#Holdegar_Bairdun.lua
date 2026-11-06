-- Original fixed named-gem exchange. This is not a learned-spell rune sequence.
local item_lib = require("items")
local gems = require("rof2_legacy_spell_data").gems
function event_say(e)
    if e.message:lower():find("hail", 1, true) then
        e.self:Say("Bring me a trade-in gem and I will give you its matching spell or tome.")
    end
end
function event_trade(e)
    if e.self:GetNPCTypeID() == 480104 then
        for slot = 1, 4 do
            local item = e.trade["item" .. slot]
            if item and item.valid then
                local id = item:GetID()
                local reward = gems[id]
                if reward and item_lib.check_turn_in(e.trade, {item1=id}) then
                    e.other:QuestReward(e.self, 0, 0, 0, 0, reward[2], 0)
                    break
                end
            end
        end
    end
    -- One gem is consumed per turn-in; unused stack quantities/items/coins return.
    item_lib.return_items(e.self, e.other, e.trade)
end
