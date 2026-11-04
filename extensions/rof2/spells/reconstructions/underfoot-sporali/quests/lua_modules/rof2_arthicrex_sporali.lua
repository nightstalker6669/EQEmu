-- Disabled source. Native loot/delivery/corpse paths remain the sole consumers.
local M = {}
local binding = require("rof2_arthicrex_task_bindings")
local spawns = require("rof2_arthicrex_sporali_spawns")
local delivery = require("achievement_native_delivery_handin")
local state = {}

local function context()
    if not binding.enabled or not binding.mission_controller_ready then return nil end
    local instance = eq.get_zone_instance_id()
    if not instance or instance == 0 then return nil end
    if state.instance ~= instance then
        if eq.get_instance_zone_id_by_id(instance) ~= binding.mission_zone
            or eq.get_instance_version_by_id(instance) ~= binding.mission_version then return nil end
        state = {instance=instance, dead={}}
    end
    if eq.get_instance_timer_by_id(instance) <= 0 then return nil end
    return instance
end

local function owner(npc)
    if not context() or not npc or not npc.valid then return nil end
    local spawn = npc:GetSpawnPointID()
    if not spawn or spawn == 0 then spawn = tonumber(npc:GetEntityVariable("sporali_spawn")) end
    local row = spawns[spawn]
    if row and row.npc == npc:GetNPCTypeID() then return row, spawn end
end

local function bind(client)
    local instance = context()
    local task = binding.tasks.sporali
    if not instance or not client or not client.valid or not client:IsTaskActive(task)
        or not eq.check_instance_by_char_id(instance, client:CharacterID())
        or client:GetSharedTaskInstanceID(task, binding.mission_zone, binding.mission_version) ~= instance then
        return false
    end
    local run = client:GetSharedTaskIdentity() -- 64-bit occurrence string, not Lua number
    if not run or run == "" or run == "0" or (state.run and run ~= state.run) then return false end
    state.run = run
    return true
end

local function killer_member(e)
    local killer = e.other
    if killer and killer.valid and not killer:IsClient() then killer = killer:GetOwner() end
    if killer and killer.valid and killer:IsClient() then
        local client = killer:CastToClient()
        if bind(client) then return client end
    end
end

local function opening_client(e)
    if e.self:GetClass() ~= 62 or not e.other or not e.other.valid or not e.other:IsClient()
        or e.damage ~= 0 or e.skill_id ~= 28 or (e.spell and e.spell.valid) then return nil end
    local client = e.other:CastToClient()
    if e.killer_id ~= client:GetID() or not bind(client) then return nil end
    return client
end

function M.spawn(e)
    local row, spawn = owner(e.self)
    if not row then return end
    e.self:SetEntityVariable("sporali_spawn", tostring(spawn))
    e.self:SetEntityVariable("sporali_instance", tostring(state.instance))
    if row.role == "grazden" or row.role == "grogald_instance" then
        e.self:SetInvul(true)
        e.self:SetSpecialAbility(24, 1)
        e.self:SetSpecialAbility(25, 1)
    end
    -- No chest invulnerability: native class62 /open invokes Death.
    -- No blanket guard protection, duplicate loot generation or respawn repair.
end

function M.say(e)
    local row = owner(e.self)
    if not row or not e.message:lower():match("%f[%a]hail%f[%A]") or not bind(e.other) then return end
    local task = binding.tasks.sporali
    if row.role == "grazden" then
        -- Two separate middle hails; select exactly one pre-event stage.
        for _, activity in ipairs({0,8,9,17}) do
            if e.other:IsTaskActivityActive(task, activity) then
                e.other:UpdateTaskActivity(task, activity, 1)
                return
            end
        end
    elseif row.role == "grogald_instance" and e.other:IsTaskActivityActive(task, 18) then
        local condition = binding.reward_chest_condition
        if not condition or state.activation_attempted then return end
        -- Activation before terminal completion. Native condition/respawn state
        -- owns an existing chest across boots; completed history never replays it.
        state.activation_attempted = true
        if eq.get_spawn_condition("arthicrex", state.instance, condition) == 0 then
            eq.spawn_condition("arthicrex", state.instance, condition, 1)
        end
        -- Setter/getter is not a durable DB receipt. No blind retry if uncertain.
        -- The Grogald hail never increments final activity18.
    end
end

function M.before_update(e)
    if e.task_id ~= binding.tasks.sporali then return 0 end
    if not bind(e.self) then return 1 end
    return 0
end

function M.allow_death(e)
    local row, spawn = owner(e.self)
    if not row then return 0 end
    if row.role == "grazden" or row.role == "grogald_instance" then return 1 end
    if row.role == "extract_chest" or row.role == "compound_chest" then
        -- Native owned-member open only; preloot before the last named is valid.
        return opening_client(e) and 0 or 1
    end
    if row.role ~= "reward_chest" then return 0 end
    local client = opening_client(e)
    local condition = binding.reward_chest_condition
    if binding.terminal_consumer ~= "native_open" or not client or not condition
        or not client:IsTaskActivityActive(binding.tasks.sporali, 18)
        or eq.get_spawn_condition("arthicrex", state.instance, condition) ~= 1
        or state.terminal_requested then return 1 end
    state.opening = {spawn=spawn, entity=e.self:GetID(), killer_entity=e.killer_id,
        character=client:CharacterID(), run=state.run, instance=state.instance}
    return 0
end

function M.death(e)
    local row, spawn = owner(e.self)
    if not row or not e.killed_entity_id or e.killed_entity_id == 0 then return end
    local key = tostring(spawn)..":"..tostring(e.killed_entity_id)
    if state.dead[key] then return end
    state.dead[key] = true -- Record premature owned deaths without future credit/repop.
    if row.role == "reward_chest" then
        local pending = state.opening
        local client = opening_client(e)
        if not pending or not client or state.terminal_requested
            or pending.spawn ~= spawn or pending.entity ~= e.killed_entity_id
            or pending.killer_entity ~= e.killer_id or pending.character ~= client:CharacterID()
            or pending.run ~= state.run or pending.instance ~= state.instance
            or not client:IsTaskActivityActive(binding.tasks.sporali, 18) then return end
        -- Mark before asynchronous native request; never replay a death or grant gear manually.
        state.terminal_requested = true
        state.opening = nil
        client:UpdateTaskActivity(binding.tasks.sporali, 18, 1)
        -- Native class62 death made the real corpse. Its normal decay/instance
        -- expiry owns gear access; no extra physical or personal item grant.
    elseif row.kill then
        local client = killer_member(e)
        if client and client:IsTaskActivityActive(binding.tasks.sporali, row.kill) then
            client:UpdateTaskActivity(binding.tasks.sporali, row.kill, 1)
        end
    end
end

function M.trade(e)
    local row = owner(e.self)
    if row and row.role == "grazden" then return delivery.trade(e) end
end

return M
