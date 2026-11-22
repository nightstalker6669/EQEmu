local por = require("rof2_por_progression")
local layout = require("rof2_deathknell_tower_layout").vertigo
local M = {}

local function delay_bat(now)
    return now + math.random(10, 12)
end

local function phase(run, index, now)
    run.phase = index
    run.deadline = now + 300
    if index >= 2 then
        eq.set_deathknell_vertigo_trap_phase(index)
        local counts = eq.get_deathknell_vertigo_disarm_counts()
        if not counts or counts.phase ~= index then return false end
    end
    if index == 2 and not run:spawn(layout.sneak.npc, layout.sneak.point, "keeper") then return false end
    if index == 3 then run.undead_at = now end
    if index == 4 then run.orc_at = now end
    return true
end

function M.start(run, client, now)
    run.bats, run.undead, run.exhausted = 0, 0, 0
    run.collective, run.pursuers = {}, {}
    run.bat_at, run.vertigo_at, run.swap_at = delay_bat(now), now + 25, now + 45
    run.collective_at = now + 25
    local keeper = run:spawn(layout.keeper.npc, layout.keeper.point, "keeper")
    if not keeper then return false end
    run.keeper_id = keeper:GetID()
    return phase(run, 1, now)
end

function M.reset()
    eq.set_deathknell_vertigo_trap_phase(0)
end

local function spawn(run, spec, role)
    local point = spec.points[math.random(#spec.points)]
    local npc = run:spawn(spec.npc, point, role)
    if npc then run:engage(npc, run:target(point, spec.range, 26)) end
    return npc
end

local function swap(first, second, instance)
    local a = {first:GetX(), first:GetY(), first:GetZ(), first:GetHeading()}
    local b = {second:GetX(), second:GetY(), second:GetZ(), second:GetHeading()}
    first:MovePCInstance(381, instance, table.unpack(b))
    second:MovePCInstance(381, instance, table.unpack(a))
end

local function chase(run)
    for id, p in pairs(run.pursuers) do
        local npc = run:actor(id)
        if not npc then return false end
        local client = por.player(npc:GetHateTop())
        local x, y = npc:GetX(), npc:GetY()
        if run:owns(client) and client:GetHP() > 0
            and math.abs(npc:GetZ() - client:GetZ()) <= 16
            and npc:CalculateDistance(client) <= layout.orc.range then
            local dx, dy = x - p.x, y - p.y
            local moved = math.sqrt(dx * dx + dy * dy)
            if moved > 0.5 and moved < 25 then p.distance = p.distance + moved end
        end
        p.x, p.y = x, y
        if p.distance >= layout.orc_distance then
            npc:Emote("collapses from exhaustion.")
            run.exhausted = run.exhausted + 1
            run.pursuers[id] = nil
            run:remove(id)
        end
    end
    return true
end

local function collective_spell(run)
    local count = 0
    local caster
    for id in pairs(run.collective) do
        caster = run:actor(id)
        if not caster then return false end
        count = count + 1
    end
    if caster then
        local spell = count >= 5 and 8568 or count >= 3 and 8567 or 8566
        caster:SpellFinished(spell, caster)
    end
    return true
end

local function quota(run)
    if run.phase == 1 then return run.bats >= 20 end
    if run.phase == 2 then
        local counts = eq.get_deathknell_vertigo_disarm_counts()
        return counts and counts.phase == 2 and counts.phase_count >= 20
    end
    if run.phase == 3 then return run.undead >= layout.undead_quota end
    return run.exhausted >= layout.orc_quota
end

function M.tick(run, now)
    if now >= run.deadline then
        if not quota(run) then return false end
        if run.phase == 4 then
            run:finish()
            return true
        end
        if not phase(run, run.phase + 1, now) then return false end
    end
    if now >= run.bat_at then
        run.bat_at = delay_bat(now)
        if not spawn(run, layout.bat, "bat") then return false end
    end
    if run.phase >= 3 and now >= run.undead_at then
        run.undead_at = now + layout.undead_interval
        local npc = spawn(run, layout.undead[math.random(#layout.undead)], "undead")
        if not npc then return false end
        run.collective[npc:GetID()] = true
    end
    if run.phase >= 3 and now >= run.collective_at then
        run.collective_at = now + 25
        if not collective_spell(run) then return false end
    end
    if run.phase == 4 then
        if now >= run.orc_at then
            run.orc_at = now + layout.orc_interval
            local npc = spawn(run, layout.orc, "orc")
            if not npc then return false end
            run.pursuers[npc:GetID()] = {x = npc:GetX(), y = npc:GetY(), distance = 0}
        end
        if not chase(run) then return false end
    end
    local players = run:players(run.info.presence, run.info.radius, run.info.height)
    if #players > 0 and now >= run.vertigo_at then
        run.vertigo_at = now + 25
        local caster = run:actor(run.keeper_id)
        if not caster then return false end
        caster:SpellFinished(8562, players[math.random(#players)])
    end
    if #players > 1 and now >= run.swap_at then
        run.swap_at = now + 45
        local first = table.remove(players, math.random(#players))
        swap(first, players[math.random(#players)], run.dz:GetInstanceID())
    end
    return true
end

function M.damage(run, e, record)
    return (record.role == "keeper" or record.role == "orc") and -1 or 0
end

function M.death(run, record, id)
    if record.role == "bat" then
        run.bats = run.bats + 1
    elseif record.role == "undead" then
        run.undead = run.undead + 1
        run.collective[id] = nil
    end
end

return M
