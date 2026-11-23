function event_zone(e)
    -- The public Innothule boundary leads to modern South Ro via point 10.
    if not e.self.valid or e.from_zone_id ~= 413 or e.from_instance_id ~= 0
        or e.from_instance_version ~= 0 or e.zone_id ~= 393 or e.instance_id ~= 0
        or e.instance_version ~= 0 then return end

    local x, y, z = e.self:GetX() - 1110, e.self:GetY() - 2570, e.self:GetZ() - 5
    if x*x + y*y <= 130*130 and math.abs(z) <= 22 then
        eq.set_data("por_tunare:innothule_south_ro:" .. e.self:CharacterID(), "1", "90s")
    end
end
