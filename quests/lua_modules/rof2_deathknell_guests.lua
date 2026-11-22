local layout = require("rof2_deathknell_tower_layout").guests
local M = {}

function M.start(run, client, now)
    run.guests, run.adds, run.halls = {}, {}, {}
    run.add_at, run.hall_at = now + layout.add_interval, now + 16
    for role, spec in ipairs(layout.bosses) do
        local npc = run:spawn(spec.npc, spec.point, "guest")
        if not npc then return false end
        run.guests[npc:GetID()] = role
        run.adds[role] = {}
    end
    return true
end

local function spawn_adds(run)
    for id, role in pairs(run.guests) do
        local boss = run:actor(id)
        if not boss then return false end
        local spec = layout.bosses[role]
        local player = run:target(spec.point, 100, 24)
        if player then
            local add = run:spawn(spec.add, spec.add_point, "add")
            if not add then return false end
            run.adds[role][add:GetID()] = true
            run:engage(add, player)
        end
    end
    return true
end

local function hall_wave(run, now)
    local passage = layout.passages[math.random(#layout.passages)]
    local start, finish = passage[1], passage[2]
    local banshee = run:spawn(layout.banshee, start, "banshee")
    if not banshee then return false end
    banshee:MoveTo(table.unpack(finish))
    run.halls[banshee:GetID()] = {finish = finish, expires = now + 20, cast = now}
    return true
end

function M.tick(run, now)
    if run.waiting_report then return true end
    if now >= run.started + layout.maximum_seconds then return false end
    if now >= run.add_at then
        run.add_at = now + layout.add_interval
        if not spawn_adds(run) then return false end
    end
    if now >= run.hall_at then
        run.hall_at = now + layout.banshee_interval
        if not hall_wave(run, now) then return false end
    end
    for id, h in pairs(run.halls) do
        local npc = run:actor(id)
        if not npc then return false end
        if now >= h.expires or npc:CalculateDistance(h.finish[1], h.finish[2], h.finish[3]) <= 4 then
            run.halls[id] = nil
            run:remove(id)
        elseif now >= h.cast then
            h.cast = now + 3
            local target = run:target({npc:GetX(), npc:GetY(), npc:GetZ()}, 18, 8)
            if target then npc:SpellFinished(8564, target) end
        end
    end
    return true
end

function M.damage(run, e, record)
    if record.role == "banshee" then return -1 end
    if record.role ~= "guest" or e.damage <= 0 then return 0 end
    local npc = e.self
    local lowest = 100 * math.max(npc:GetHP() - e.damage, 0) / npc:GetMaxHP()
    local highest = 0
    for id in pairs(run.guests) do
        local other = run:actor(id)
        if other and id ~= npc:GetID() then highest = math.max(highest, other:GetHPRatio()) end
    end
    if highest - lowest > 10 then
        npc:SetHP(math.floor(npc:GetMaxHP() * highest / 100))
        return -1
    end
    return 0
end

function M.death(run, record, id)
    if record.role == "add" then
        for _, adds in pairs(run.adds) do adds[id] = nil end
    elseif record.role == "guest" then
        local role = run.guests[id]
        for add_id in pairs(run.adds[role]) do run:remove(add_id) end
        run.adds[role], run.guests[id] = nil, nil
        if not next(run.guests) then
            for actor in pairs(run.halls) do run:remove(actor) end
            run.halls = {}
            run.waiting_report = true
        end
    end
end

return M
