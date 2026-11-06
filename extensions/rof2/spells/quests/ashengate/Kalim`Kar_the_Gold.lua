-- Original TSS Glowing Obsidian Rune exchange; sources in legacy-exchanges.json.
local runes = require("rof2_spell_runes")
local data = require("rof2_legacy_spell_data")
function event_say(e)
    if e.message:lower():find("hail", 1, true) then
        e.self:Say("Bring me Glowing Obsidian Runes from the Reliquary and I can improve your magic.")
    end
end
function event_trade(e)
    runes.hand_in_catalogue(e, 406000, data.tss, 71)
end
