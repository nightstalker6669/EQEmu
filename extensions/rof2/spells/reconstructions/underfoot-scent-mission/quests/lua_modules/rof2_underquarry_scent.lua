-- Draft owned mission controller. No journal, manual item grant, or shared
-- core replacement. Publish/install only with the complete guarded component.
local M = {}
local access = require("rof2_underquarry_scent_access")
local spawns = require("rof2_underquarry_scent_spawns")
local delivery = require("achievement_native_delivery_handin")
local TASK, LADY, ALLY, RECEIVER, CHEST = 920136, 482900, 482901, 482910, 482911
local state = {}

local function context()
    local instance = eq.get_zone_instance_id()
    if not instance or instance == 0 then return nil end
    if state.instance ~= instance then
        if access.instance() ~= instance then return nil end
        state = {instance=instance, dead={}, phase="awaiting"}
    end
    return instance
end

local function owner(npc)
    local instance = context()
    if not instance or not npc or not npc.valid then return nil end
    local spawn = npc:GetSpawnPointID()
    if not spawn or spawn == 0 then
        spawn = tonumber(npc:GetEntityVariable("scent_spawn"))
    end
    local row = spawns[spawn]
    if row and row.npc == npc:GetNPCTypeID() then return row, spawn end
end

local function each_owned(fn)
    local list = eq.get_entity_list():GetNPCList()
    for npc in list.entries do
        local row, spawn = owner(npc)
        if row then fn(npc, row, spawn) end
    end
end

local function named(npc_id)
    local found
    each_owned(function(npc, row)
        if row.npc == npc_id then found = npc end
    end)
    return found
end

local function bind(client)
    local instance = context()
    if not instance or not access.member(client) or not client:IsTaskActive(TASK) then return false end
    if client:GetSharedTaskInstanceID(TASK, 482, 1) ~= instance then return false end
    local run = client:GetSharedTaskIdentity() -- decimal string, never Lua number
    if not run or run == "" or (state.run and state.run ~= run) then return false end
    state.run = run
    each_owned(function(npc, _, spawn)
        npc:SetEntityVariable("scent_instance", tostring(instance))
        npc:SetEntityVariable("scent_spawn", tostring(spawn))
        npc:SetEntityVariable("scent_run", run)
    end)
    return true
end

local function member_for_update()
    local list = eq.get_entity_list():GetClientList()
    for client in list.entries do
        if client:GetHP() > 0 and bind(client) then return client end
    end
end

local function stop_ally()
    local ally = named(ALLY)
    if ally then
        ally:SetSpecialAbility(24, 1) -- AggroImmunity: prevents new hate
        ally:WipeHateList()
    end
end

local function start_combat(client)
    if state.phase == "combat" or state.phase == "boss_alone" then return end
    if not bind(client) or not client:IsTaskActivityActive(TASK, 18) then return end
    local lady, ally = named(LADY), named(ALLY)
    if not lady or not ally then return end -- no guessed replacement spawns
    state.phase = "combat"
    lady:SetInvul(false)
    lady:SetSpecialAbility(24, 0)
    lady:SetSpecialAbility(25, 0)
    ally:SetInvul(true) -- labelled support-role protection default
    ally:SetSpecialAbility(24, 0)
    ally:SetSpecialAbility(25, 1) -- BeingAggroImmunity: Lady cannot acquire ally hate
    ally:SetSpecialAbility(49, 1) -- ClientAggroImmunity: ally never acquires player hate
    ally:WipeHateList()
    ally:AddToHateList(lady, 1000)
    lady:AddToHateList(client, 1000)
end

function M.spawn(e)
    local row, spawn = owner(e.self)
    if not row then return end
    e.self:SetEntityVariable("scent_spawn", tostring(spawn))
    e.self:SetEntityVariable("scent_instance", tostring(state.instance))
    if state.run then e.self:SetEntityVariable("scent_run", state.run) end
    if row.npc == LADY or row.npc == ALLY or row.npc == RECEIVER then
        e.self:SetInvul(true)
        e.self:SetSpecialAbility(24, 1)
        e.self:SetSpecialAbility(25, 1)
    end
    if row.npc == LADY then
        eq.set_next_hp_event(50) -- NPC event context, not player callback context
    elseif row.npc == CHEST then
        -- Never protect the chest with invulnerability: native /open calls Death.
        local seconds = math.min(600, eq.get_instance_timer_by_id(state.instance))
        if seconds <= 0 then e.self:Depop(true); return end
        e.self:SetEntityVariable("scent_close_epoch", tostring(os.time()+seconds))
        eq.set_timer("scent_chest_window", seconds*1000)
    end
    local client = member_for_update()
    if client and client:IsTaskActivityActive(TASK, 18) then start_combat(client) end
end

function M.enter(e)
    if not bind(e.self) then return end
    if e.self:IsTaskActivityActive(TASK, 18) then
        start_combat(e.self)
    elseif e.self:IsTaskActivityActive(TASK, 19) then
        state.phase = "final_delivery"
        stop_ally()
    end
    -- Completed-task history never activates a chest or recreates dead roles.
end

function M.say(e)
    local row = owner(e.self)
    if not row or not e.message:lower():match("%f[%a]hail%f[%A]") or not bind(e.other) then return end
    if row.npc == LADY and e.other:IsTaskActivityActive(TASK, 0) then
        e.other:UpdateTaskActivity(TASK, 0, 1)
    elseif row.npc == ALLY and e.other:IsTaskActivityActive(TASK, 17) then
        e.other:UpdateTaskActivity(TASK, 17, 1)
    end
    -- Side effects wait for confirmed native updates, not these requests.
end

function M.before_update(e)
    if e.task_id ~= TASK then return 0 end
    if not bind(e.self) then return 1 end
    if e.activity_id == 19 and e.count > 0 then
        state.pending_final = {instance=state.instance, run=state.run}
    end
    return 0
end

function M.update(e)
    if e.task_id ~= TASK or not e.count or e.count <= 0 or not context() then return end
    if e.activity_id == 0 then
        if bind(e.self) then state.phase = "parts" end
    elseif e.activity_id == 17 then
        start_combat(e.self)
    elseif e.activity_id == 19 then
        -- The active SharedTaskID can already be cleared here. Validate the
        -- captured occurrence and surviving native membership, never rebind it.
        local pending = state.pending_final
        if not pending or pending.instance ~= state.instance or pending.run ~= state.run
            or not access.member(e.self) or state.chest_requested then return end
        state.chest_requested = true
        state.pending_final = nil
        state.phase = "complete"
        stop_ally()
        if eq.get_spawn_condition("underquarry", state.instance, 6201) == 0 then
            eq.spawn_condition("underquarry", state.instance, 6201, 1)
        end
        -- A getter is not a durable commit receipt. No retry/regrant/replay.
    end
end

function M.hp(e)
    local row = owner(e.self)
    if row and row.npc == LADY and e.hp_event == 50 and state.phase == "combat" then
        state.phase = "boss_alone"
        stop_ally()
    end
end

function M.hate(e)
    local row = owner(e.self)
    if not row or row.npc ~= ALLY or not e.joined or not e.other then return end
    if e.other:GetNPCTypeID() ~= LADY then
        -- Only this owned support actor's hate list is changed. Never clear
        -- another NPC's/player's hate or target a player-owned pet as the ally.
        e.self:WipeHateList()
        if state.phase == "combat" then
            local lady = named(LADY)
            if lady then e.self:AddToHateList(lady, 1000) end
        end
    end
end

function M.allow_death(e)
    local row = owner(e.self)
    if not row then return 0 end
    if row.npc == RECEIVER or row.npc == ALLY then return 1 end
    if row.npc == LADY and state.phase ~= "combat" and state.phase ~= "boss_alone" then return 1 end
    if row.npc == CHEST then
        local close = tonumber(e.self:GetEntityVariable("scent_close_epoch")) or 0
        if close <= os.time() or not e.other or not e.other:IsClient()
            or not access.member(e.other:CastToClient()) then return 1 end
    end
    return 0
end

function M.death(e)
    local row, spawn = owner(e.self)
    if not row or not e.killed_entity_id or e.killed_entity_id == 0 then return end
    local key = tostring(spawn)..":"..tostring(e.killed_entity_id)
    if state.dead[key] then return end
    state.dead[key] = true
    if row.npc == CHEST then
        local corpse = eq.get_entity_list():GetCorpseByID(e.killed_entity_id)
        local close = tonumber(e.self:GetEntityVariable("scent_close_epoch")) or os.time()
        if corpse and corpse.valid then
            corpse:SetDecayTimer(math.max(1, close-os.time())*1000)
        end
    elseif row.npc == LADY then
        state.phase = "boss_dead"
        stop_ally()
    elseif row.kill then
        local client = member_for_update()
        if client and e.self:GetEntityVariable("scent_run") == state.run
            and client:IsTaskActivityActive(TASK, row.kill) then
            client:UpdateTaskActivity(TASK, row.kill, 1) -- one member, one owned death
        end
    end
end

function M.timer(e)
    local row = owner(e.self)
    if row and row.npc == CHEST and e.timer == "scent_chest_window" then
        eq.stop_timer(e.timer)
        e.self:SetSpecialAbility(53, 1) -- OpenImmunity for a stale /open request
        e.self:Depop(true) -- native owned spawn timer; never toggle condition back
    end
end

function M.trade(e)
    return delivery.trade(e) -- native delivery is the only item consumer
end

return M
