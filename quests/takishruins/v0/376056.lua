function event_say(e)
    local zone = eq.get_zone()
    if zone:GetZoneID() ~= 376 or zone:GetInstanceID() ~= 0 or e.self:GetSpawnPointID() ~= 106865 then return end
    local words = e.message:lower()
    if words == "hail" then
        e.self:Say("Our greatest " .. eq.say_link("mistakes", false) .. " are all I remember.")
    elseif words == "mistakes" then
        eq.task_selector({920310})
    end
end
