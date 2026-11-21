function event_combat(e)
    if e.joined then
        eq.set_timer("por_salers_curse",30000)
        eq.set_timer("por_salers_beckon",18000)
    else
        eq.stop_timer("por_salers_curse")
        eq.stop_timer("por_salers_beckon")
    end
end

function event_timer(e)
    local target=e.self:GetTarget()
    if not target or not target.valid then return end
    if e.timer=="por_salers_curse" then e.self:CastSpell(8957,target:GetID())
    elseif e.timer=="por_salers_beckon" then e.self:CastSpell(7046,target:GetID()) end
end
