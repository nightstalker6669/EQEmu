-- Source reconstruction. Exact accepted occurrence and returned actor handles only.
local M = {}
local b = require("rof2_arthicrex_task_bindings")
local runs, offers, serial = {}, {}, 0
local timer = "sporali_desperate_owned_poll"
-- Configurable LOCAL defaults; these are not recovered original values/predicates.
local policy = {offer_seconds=120, hail_seconds=180, poll_ms=1000,
    idle_scope="whole_wave_unengaged", warning_seconds=120, fail_seconds=300}
local waves = {
    {{1153.3003,-171.6616,6.5969},{1165.3003,-171.6616,6.5969},{1177.3003,-171.6616,6.5969}},
    {{868,-260,5},{880,-260,5},{892,-260,5}},
    {{928,-160,5},{940,-160,5},{952,-160,5}},
}

local function enabled()
    return b.enabled and b.desperate_controller_ready and eq.get_zone_id() == 485
        and eq.get_zone_instance_id() == 0
end
local function manager(npc)
    return npc and npc.valid and npc:GetNPCTypeID() == b.npc_types.mardan
        and npc:GetSpawnPointID() == b.mardan_spawn
end
local function identity(client)
    if not client or not client.valid or not client:IsTaskActive(b.tasks.desperate) then return nil end
    local id = client:GetSharedTaskIdentity()
    if id and id ~= "" and id ~= "0" then return id end
end
local function member(run)
    local clients = eq.get_entity_list():GetClientList()
    for client in clients.entries do
        if identity(client) == run.id then return client end
    end
end
local function actual_actor(actor)
    local mob = eq.get_entity_list():GetMobID(actor.entity)
    if mob and mob.valid and mob:GetNPCTypeID() == b.agent_npc_type
        and mob:GetEntityVariable("sporali_agent_token") == actor.token then return mob end
end
local function cleanup(run)
    for _, actor in pairs(run.actors) do
        local mob = actual_actor(actor)
        if mob then mob:Depop(false) end
    end
end
local function fail(run, reason)
    run.failed = reason
    cleanup(run)
    local client = member(run)
    if client and not run.failure_requested then
        run.failure_requested = true -- one native shared failure request, never per-member replay
        client:Message(13, "A Desperate Deed failed: "..reason)
        client:FailTask(b.tasks.desperate)
    end
end
local function start_wave(run, client, wave)
    if run.failed or run.spawning or run.started[wave] then return end
    if identity(client) ~= run.id then return fail(run, "requester no longer in this task occurrence") end
    run.spawning = true
    run.started[wave] = true -- no retry after partial native spawn
    run.wave, run.kills, run.actors = wave, 0, {}
    run.phase, run.last_engaged, run.warned = "spawning", os.time(), false
    for index, point in ipairs(waves[wave]) do
        serial = serial + 1
        local token = run.id..":"..wave..":"..index..":"..os.time()..":"..serial
        local mob = eq.spawn2(b.agent_npc_type, 0, 0, point[1], point[2], point[3], 0)
        if not mob or not mob.valid or mob:GetID() == 0 or mob:GetNPCTypeID() ~= b.agent_npc_type then
            run.spawning = false
            return fail(run, "partial owned wave spawn; reconciliation required")
        end
        local actor = {entity=mob:GetID(), token=token, wave=wave, dead=false}
        run.actors[actor.entity] = actor
        mob:SetEntityVariable("sporali_agent_run", run.id)
        mob:SetEntityVariable("sporali_agent_token", token)
        -- Per-actor native lifetime prevents abandoned agents surviving a Lua reload.
        -- This cleans only this returned actor; it never recreates an occurrence.
        eq.set_timer("sporali_agent_lease", math.max(1, run.deadline-os.time())*1000, mob)
    end
    run.spawning, run.phase = false, "wave"
    -- Register all three real handles before any can become progress eligible.
    for _, actor in pairs(run.actors) do
        local mob = actual_actor(actor)
        if not mob then return fail(run, "owned actor lost during spawn") end
        mob:AddToHateList(client, 1)
    end
end

function M.say(e)
    if not enabled() or not manager(e.self) then return end
    local message, client = e.message:lower(), e.other
    if message:match("%f[%a]ready%f[%A]") then
        if not client:IsTaskCompleted(b.tasks.abandon) or not client:IsTaskCompleted(b.tasks.fallen)
            or client:IsTaskActive(b.tasks.desperate) then return end
        offers[client:CharacterID()] = {until_time=os.time()+policy.offer_seconds}
        eq.task_selector({b.tasks.desperate}) -- no spawn on an offer
        return
    end
    if not message:match("%f[%a]hail%f[%A]") then return end
    local run = runs[identity(client)]
    if not run or run.failed or run.hail_pending then return end
    local activity = run.phase == "hail2" and 1 or run.phase == "hail3" and 3
        or run.phase == "normal" and 7 or nil
    if activity and client:IsTaskActivityActive(b.tasks.desperate, activity) then
        run.hail_pending = {activity=activity, character=client:CharacterID()}
        client:UpdateTaskActivity(b.tasks.desperate, activity, 1)
    end
end

function M.accepted(e)
    if not enabled() or e.task_id ~= b.tasks.desperate then return end
    local id = identity(e.self)
    if not id or runs[id] then return end
    local offer = offers[e.self:CharacterID()]
    if not offer or offer.until_time < os.time() then
        e.self:Message(13, "This task occurrence has no owned wave controller; reconciliation is required.")
        return -- Never reconstruct actors from completed history or an unrelated acceptance.
    end
    offers[e.self:CharacterID()] = nil
    local npc = eq.get_entity_list():GetNPCByNPCTypeID(b.npc_types.mardan)
    if not manager(npc) then return end
    local run = {id=id, deadline=os.time()+3600, actors={}, started={}, phase="accepted"}
    runs[id] = run
    eq.set_timer(timer, policy.poll_ms, npc)
    start_wave(run, e.self, 1)
end

function M.before_update(e)
    if not enabled() or e.task_id ~= b.tasks.desperate then return 0 end
    local run = runs[identity(e.self)]
    return run and not run.failed and 0 or 1
end

function M.updated(e)
    if not enabled() or e.task_id ~= b.tasks.desperate then return end
    local run = runs[identity(e.self)]
    -- Completion may clear native shared identity before the final notification.
    if not run and e.activity_id == 7 then
        for _, candidate in pairs(runs) do
            if candidate.hail_pending and candidate.hail_pending.activity == 7
                and candidate.hail_pending.character == e.self:CharacterID() then run=candidate; break end
        end
    end
    if not run or run.failed then return end
    local wave_goal = run.wave and (run.wave-1)*2
    if e.activity_id == wave_goal and e.count >= 3 and run.kills == 3 and run.phase == "wave" then
        if run.wave == 3 then run.phase="normal" else
            run.phase = run.wave == 1 and "hail2" or "hail3"
            run.hail_deadline = os.time()+policy.hail_seconds
        end
    elseif run.hail_pending and run.hail_pending.activity == e.activity_id and e.count >= 1 then
        local activity = run.hail_pending.activity
        run.hail_pending = nil
        if activity == 1 or activity == 3 then start_wave(run, e.self, activity == 1 and 2 or 3)
        elseif activity == 7 then cleanup(run); runs[run.id]=nil end
    end
end

function M.agent_death(e)
    if not enabled() or e.self:GetNPCTypeID() ~= b.agent_npc_type then return end
    local run = runs[e.self:GetEntityVariable("sporali_agent_run")]
    local actor = run and run.actors[e.killed_entity_id]
    if not actor or actor.dead or run.failed or run.phase ~= "wave"
        or actor.token ~= e.self:GetEntityVariable("sporali_agent_token") then return end
    actor.dead = true -- exact original entity/token, once before any native update
    local killer = e.other
    if killer and killer.valid and not killer:IsClient() then killer = killer:GetOwner() end
    if not killer or not killer.valid or not killer:IsClient()
        or identity(killer:CastToClient()) ~= run.id then
        return fail(run, "owned wave killed by a nonmember")
    end
    local client = killer:CastToClient()
    local activity = (run.wave-1)*2
    if not client:IsTaskActivityActive(b.tasks.desperate, activity) then
        return fail(run, "owned wave died outside its active stage")
    end
    run.kills = run.kills+1
    client:UpdateTaskActivity(b.tasks.desperate, activity, 1)
end

function M.agent_timer(e)
    if e.timer == "sporali_agent_lease" and e.self:GetNPCTypeID() == b.agent_npc_type
        and e.self:GetEntityVariable("sporali_agent_token") ~= "" then e.self:Depop(false) end
end

function M.timer(e)
    if not enabled() or not manager(e.self) or e.timer ~= timer then return end
    local now, any = os.time(), false
    for _, run in pairs(runs) do
        any = true
        if run.failed then
            if not run.failure_requested then fail(run, run.failed) end
        elseif now >= run.deadline then fail(run, "one-hour task limit")
        elseif (run.phase == "hail2" or run.phase == "hail3") and now >= run.hail_deadline then
            fail(run, "interwave hail deadline (local setting)")
        elseif run.phase == "wave" then
            local engaged = false
            for _, actor in pairs(run.actors) do
                if not actor.dead then
                    local mob = actual_actor(actor)
                    if not mob then fail(run, "owned actor disappeared without death confirmation"); break end
                    if mob:IsEngaged() or mob:GetHateListCount() > 0 then engaged=true end
                end
            end
            if not run.failed then
                if engaged then run.last_engaged=now; run.warned=false end
                local idle = now-run.last_engaged
                if idle >= policy.fail_seconds then fail(run, "wave idle for five minutes")
                elseif idle >= policy.warning_seconds and not run.warned then
                    run.warned=true
                    local client=member(run)
                    if client then client:Message(13, "The agents are losing patience. Engage the wave.") end
                end
            end
        end
        if run.failed and (run.failure_requested or now >= run.deadline) then runs[run.id]=nil end
    end
    if not any then eq.stop_timer(timer, e.self) end
end
return M
