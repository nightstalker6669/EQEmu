local timer = "peacekeeper_spell"

return function(spell_id)
    local function combat(e)
        if e.joined == true or tonumber(e.joined) == 1 then
            -- The spell is original. The repeat interval is a local reconstruction.
            eq.set_timer(timer, 30000)
        else
            eq.stop_timer(timer)
        end
    end

    local function cast(e)
        if e.timer ~= timer then
            return
        end
        if not e.self:IsEngaged() then
            eq.stop_timer(timer)
            return
        end
        local target = e.self:GetHateTop()
        if target.valid and not e.self:IsCasting() then
            e.self:CastSpell(spell_id, target:GetID())
        end
    end

    return combat, cast
end
