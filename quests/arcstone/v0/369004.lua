local function is_scribe(npc)
    local zone = eq.get_zone()
    return zone:GetZoneID() == 369 and zone:GetInstanceID() == 0 and
        npc:GetNPCTypeID() == 369004 and npc:GetSpawnPointID() == 103451
end

function event_say(e)
    if not is_scribe(e.self) then return end
    local words = e.message:lower()
    if words == "hail" then
        e.self:Say("I have work to finish. Could you " .. eq.say_link("help", false) .. " me?")
    elseif words == "help" then
        e.self:Say("Bring me three pieces of parchment and a quill, and I will sell you my copied spells.")
    end
end

function event_trade(e)
    if is_scribe(e.self) and e.self:CheckHandin(e.other, {}, {[13063]=3, [13051]=1}, {}) then
        e.other:RewardFaction(1062, 2400)
        e.self:Say("That will do. You may look through my spellbooks.")
    end
    e.self:ReturnHandinItems(e.other)
end
