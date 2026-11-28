local towers = {
    { x = 757.772, y = -437.5, base_z = 250, peacekeeper = 730974, site = { 871.5, -502.875, 556.33776855, 449 } },
    { x = -759.248, y = 436.648, base_z = 300, peacekeeper = 730975, site = { -803.75, 398, 449.4830017, 111 } },
    { x = 427.5, y = 757.772, base_z = 350, peacekeeper = 730976, site = { 463, 835, 514.5, 178 } },
    { x = -437.5, y = -757.772, base_z = 200, peacekeeper = 730977, site = { -451.875, -840, 351.8420105, 159 } }
}

function event_death_zone(e)
    local npc = e.killed
    local killer = e.other
    if not npc.valid or npc:IsPet() or npc:GetSpawnPointID() == 0
        or not killer.valid or not killer:IsClient() or killer:CastToClient():GetGM() then
        return
    end

    local tower = towers[eq.get_pillars_hunter_phase() + 1]
    if not tower then
        return
    end

    -- Use the static home rather than the death position so pulling outside a
    -- tower does not change its faction. The height floor excludes the moat.
    local dx = npc:GetSpawnPointX() - tower.x
    local dy = npc:GetSpawnPointY() - tower.y
    if dx * dx + dy * dy > 240 * 240 or npc:GetSpawnPointZ() < tower.base_z then
        return
    end
    if eq.get_entity_list():GetNPCByNPCTypeID(tower.peacekeeper).valid then
        return
    end

    -- The original odds are unavailable. Keep this local rate on the live
    -- death path and leave the stock tower spawn groups untouched.
    if Random.Roll(10) then
        eq.unique_spawn(tower.peacekeeper, 0, 0, unpack(tower.site))
    end
end
