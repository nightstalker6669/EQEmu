function event_spawn(e) eq.set_next_hp_event(80) end

function event_hp(e)
    local n=e.self
    local target=n:GetTarget()
    if not target or not target.valid then return end
    if e.hp_event==80 then eq.set_next_hp_event(40) end
    for _,direction in ipairs({-1,1}) do
        local guard=eq.spawn2(376965,0,0,n:GetX(),n:GetY()+direction*4,n:GetZ(),0)
        if guard.valid then guard:AddToHateList(target,1) end
    end
end
