local runtime = require("rof2_dodh_raid_runtime")
local layout = require("rof2_deathknell_tower_layout").bells
local M = {}

local function make_actors(run, roles, role)
    for index, entry in ipairs(roles) do
        local npc = run:spawn(entry.npc, entry.point, role)
        if not npc then return false end
        if role == "watcher" then
            run.watchers[npc:GetID()] = {index = index, steps = 0}
        else
            run.enforcers[npc:GetID()] = true
        end
        run:engage(npc, run:target(entry.point, 115, 25))
    end
    return true
end

function M.start(run, client, now)
    run.phase = "watchers"
    run.watchers, run.enforcers = {}, {}
    run.ring_at, run.add_at = now + layout.interval, now + layout.add_interval
    run.watch_until, run.enforce_at = now + 900, now
    return make_actors(run, layout.watchers, "watcher")
end

local function ring(run, watcher)
    local bell = runtime.placed(layout.bell, run.dz)
    local caster = bell or watcher
    caster:SpellFinished(8559, caster)
end

local function advance_watchers(run)
    for id, state in pairs(run.watchers) do
        local npc = run:actor(id)
        if not npc then return false end
        local damage = 100 * (npc:GetMaxHP() - npc:GetHP()) / npc:GetMaxHP()
        if damage >= layout.push_pct then
            state.steps = math.max(state.steps - 1, 0)
        elseif damage < layout.hold_pct then
            state.steps = state.steps + 1
        end
        if state.steps == layout.steps_to_bell then
            ring(run, npc)
            state.steps = 0
        end
        local origin = layout.watchers[state.index].point
        local stop = layout.watchers[state.index].stop
        local toward = state.steps / layout.steps_to_bell
        npc:GMMove(origin[1] + (stop[1] - origin[1]) * toward,
            origin[2] + (stop[2] - origin[2]) * toward, origin[3], origin[4])
        npc:SetHP(npc:GetMaxHP())
    end
    return true
end

function M.tick(run, now)
    if now - run.started >= layout.maximum_seconds then return false end
    if now >= run.add_at then
        run.add_at = now + layout.add_interval
        local point = layout.add_points[math.random(#layout.add_points)]
        local which = math.random(4) == 1 and layout.follower or layout.vampire
        local add = run:spawn(which, point, "vampire")
        if not add then return false end
        run:engage(add, run:target(point, 190, 32))
    end
    if run.phase == "watchers" then
        if now >= run.watch_until then
            for id in pairs(run.watchers) do run:remove(id) end
            run.watchers = {}
            run.phase = "enforcers"
            run.enforce_at = now + 15
            return make_actors(run, layout.enforcers, "enforcer")
        end
        if now >= run.ring_at then
            run.ring_at = now + layout.interval
            return advance_watchers(run)
        end
    elseif now >= run.enforce_at then
        run.enforce_at = now + layout.enforcer_interval
        for id in pairs(run.enforcers) do
            local enforcer = run:actor(id)
            if not enforcer then return false end
            enforcer:SpellFinished(7304, enforcer)
            local target = run:target({enforcer:GetX(),enforcer:GetY(),enforcer:GetZ()}, 150, 28)
            if target then enforcer:SpellFinished(7917, target) end
        end
    end
    return true
end

function M.damage(run, e, record)
    if record.role ~= "watcher" then return 0 end
    if e.spell_id > 0 and e.spell_id ~= 65535 and not e.other:CombatRange(e.self) then return -1 end
    if e.damage >= e.self:GetHP() then
        local allowed = e.self:GetHP() - 1
        return allowed > 0 and allowed or -1
    end
    return 0
end

function M.death(run, record, id)
    if record.role == "enforcer" then
        run.enforcers[id] = nil
        if not next(run.enforcers) then run:finish() end
    end
end

return M
