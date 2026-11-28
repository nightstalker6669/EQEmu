local missions = {
    [304101815] = { 920380, 700, 304101837, 6, "The runners left a clear trail but cannot signal each other. Check the two relay posts, then stop the patrol before it reaches the road." },
    [304101881] = { 920381, 701, 304101904, 7, "If the lights count the same visitor twice, the watch cannot know which door to close. Check the marks I left, then meet me where we started." },
    [304101932] = { 920382, 703, 304101949, 7, "Keep the new trays out of the runoff. The survey flags mark dry ground. Clear the burrowers first, then find the hollow where the water backed up." },
    [304102017] = { 920383, 705, 304102021, 6, "I can work with two clear routes. The stone gives back a different sound on each, so check both bearings before you return." },
    [304102022] = { 920384, 704, 304102026, 6, "I have a blank counterfoil ready. Look at both shelves. The marks you bring back should agree, even if the pages refuse to." },
    [304102027] = { 920385, 706, 304102031, 7, "We can wave the boat in once the approach is honest. The wind and the stairs make two different routes down to the water." },
    [304102032] = { 920386, 702, 304102036, 7, "There is no clean account of the watch. Visit the three marks and bring me a route that could belong to a real person." },
    [304102037] = { 920387, 709, 304102041, 6, "An island cannot pull north and south at once. Make a sighting at both stations. The rest of us will keep the camp open." },
    [304102042] = { 920388, 710, 304102046, 7, "The reflections are repeating a courier's route. Stand on both observation marks, and you should see where it stops being a reflection." },
    [304102047] = { 920389, 711, 304102051, 7, "Your report is the only offering I need. Keep a way back open while you find the second turn. The gate patrol is expecting a straight approach." },
}

local M = {}

function M.say(e)
    local mission = missions[e.self:GetNPCTypeID()]
    if not mission or not e.other.valid or not e.message:lower():match("^hail") then
        return
    end
    local task, zone_id, chest_id, terminal, brief = unpack(mission)
    local zone = eq.get_zone()
    local instance = zone:GetInstanceID()
    if zone:GetZoneID() ~= zone_id or zone:GetInstanceVersion() ~= 20 or instance == 0 or
        e.other:GetSharedTaskInstanceID(task, zone_id, 20) ~= instance then
        return
    end

    if e.other:IsTaskActivityActive(task, terminal - 1) or e.other:IsTaskActivityActive(task, terminal) then
        local chest = eq.get_entity_list():GetNPCByNPCTypeID(chest_id)
        local run = e.other:GetSharedTaskIdentity()
        if not chest.valid or run == "" then
            e.self:Say("The supply chest is unavailable.")
            return
        end
        local mark = chest:GetEntityVariable("hot_group_mission")
        if mark ~= "" and mark ~= run then
            return
        end
        chest:SetEntityVariable("hot_group_mission", run)
        e.self:Say("I have your report. Target the supply chest and use Open to finish the mission.")
    else
        e.self:Say(brief)
    end
end

return M
