-- Spell acquisition only. Reward order and sources are in the rune catalogue.
local runes = require("rof2_spell_runes")

function event_say(e)
    if e.message:lower():find("hail", 1, true) then
        e.self:Say("Bring me the raid runes and I will exchange them for spells and disciplines you have yet to learn.")
    end
end

function event_trade(e)
    runes.hand_in(e, "underfoot")
end
