-- Original SoD rune exchange; withheld class/rune sequences return their input.
local runes = require("rof2_spell_runes")
local data = require("rof2_sod_rune_data")
function event_say(e)
    if e.message:lower():find("hail", 1, true) then
        e.self:Say("Bring me unusual runes from the time rift and I can concentrate their magic.")
    end
end
function event_trade(e)
    runes.hand_in_catalogue(e, 219900, data, 80)
end
