local deathknell = require("rof2_por_deathknell")
local runtime = require("rof2_dodh_raid_runtime")
local por = require("rof2_por_progression")
local layout = require("rof2_deathknell_tower_layout")
local events = {
    require("rof2_deathknell_bells"),
    require("rof2_deathknell_vertigo"),
    require("rof2_deathknell_guests"),
    (require("rof2_deathknell_choir")),
}

local binding = deathknell.raids.deathknell
local M = {}
local scene
local tick_timer = "por_deathknell_tower_tick"
local marker = "por_deathknell_tower_event"
local chest_uuid = "por_deathknell_chest_uuid"
local chest_event = "por_deathknell_chest_event"
local vertigo_trapdoor = {62, 0, 0}
local vertigo_landing = {62, 0, -46}
local guests_trapdoor = {0, 0, -499}
local guests_landing = {0, 0, -546}

local function scope(client)
    local dz = runtime.scope(client, binding)
    if dz and dz:GetSecondsRemaining() > 0 then return dz end
end

local function near(actor, point, range, height)
    local x, y = actor:GetX() - point[1], actor:GetY() - point[2]
    return x * x + y * y <= range * range and math.abs(actor:GetZ() - point[3]) <= height
end

local function next_stage(dz)
    for index, entry in ipairs(layout.events) do
        if not dz:HasLockout(entry.name) then return index end
    end
end

local function initialize(dz)
    if scene then return scene.uuid == dz:GetUUID() and scene or nil end
    local uuid = dz:GetUUID()
    local chest_names = {}
    for _, entry in ipairs(layout.events) do
        chest_names[entry.chest] = entry.name
        if dz:HasLockout(entry.name) then dz:SetLootEventByNPCTypeID(entry.chest, entry.name) end
    end
    for actor in eq.get_entity_list():GetNPCList().entries do
        local mark = actor:GetEntityVariable(marker)
        local event = chest_names[actor:GetNPCTypeID()]
        -- Stale in-progress enemies have no encounter state after a worker reload.
        if mark:sub(1, #uuid + 1) == uuid .. ":" then
            if not event or actor:GetEntityVariable(chest_uuid) ~= uuid
                or actor:GetEntityVariable(chest_event) ~= event or not dz:HasLockout(event) then
                actor:Depop(false)
            end
        end
    end
    scene = {uuid = uuid, dz = dz, generation = 0, tick = -1}
    return scene
end

local Encounter = {}
Encounter.__index = Encounter

function Encounter:actor(id)
    local record = self.actors[id]
    return record and runtime.owned(self.mark, id, record.npc)
end

function Encounter:spawn(npc_type, location, role)
    local npc = runtime.spawn_owned(npc_type, location, self.mark)
    if npc then
        npc:SetEntityVariable(marker, self.mark)
        self.actors[npc:GetID()] = {npc = npc_type, role = role}
    end
    return npc
end

function Encounter:remove(id)
    local npc = self:actor(id)
    self.actors[id] = nil
    if npc then npc:Depop(false) end
end

function Encounter:players(point, range, height)
    local players = {}
    for client in eq.get_entity_list():GetClientList().entries do
        local dz = scope(client)
        if dz and dz:GetUUID() == self.uuid and client:GetHP() > 0
            and (not point or near(client, point, range, height)) then
            players[#players + 1] = client
        end
    end
    return players
end

function Encounter:target(point, range, height)
    local players = self:players(point, range, height)
    return #players > 0 and players[math.random(#players)] or nil
end

function Encounter:engage(npc, client)
    if npc and client then npc:AddToHateList(client, 1) end
end

function Encounter:owns(client)
    local dz = scope(client)
    return dz and dz:GetUUID() == self.uuid
end

function Encounter:reset()
    if self.controller.reset then self.controller.reset(self) end
    for id in pairs(self.actors) do self:remove(id) end
    scene.active = nil
end

function Encounter:finish()
    -- Keep the success pending if the chest cannot be created this tick.
    self.success = true
end

local function settle(run)
    if run.dz:HasLockout(run.info.name) then
        run:reset()
        return
    end
    local chest = runtime.spawn_owned(run.info.chest, run.info.reward, run.mark)
    if not chest then return end
    chest:SetEntityVariable(marker, run.mark)
    chest:SetEntityVariable(chest_uuid, run.uuid)
    chest:SetEntityVariable(chest_event, run.info.name)
    chest:SetEntityVariable("por_deathknell:harp_chest", run.uuid .. ":" .. run.info.name)
    chest:AddItem(52602, 1, false)
    run.dz:SetLootEventByNPCTypeID(run.info.chest, run.info.name)
    run.dz:AddLockout(run.info.name, layout.lockout_seconds)
    if not run.dz:HasLockout(run.info.name) then
        chest:Depop(false)
        return
    end
    run:reset()
end

local function start(client, index)
    local dz = scope(client)
    local current = dz and initialize(dz)
    if not current or current.active or next_stage(dz) ~= index then return false end
    current.generation = current.generation + 1
    local now = os.time()
    local run = setmetatable({
        uuid = current.uuid, dz = dz, stage = index, mark = current.uuid .. ":tower:" .. index .. ":" .. current.generation,
        info = layout.events[index], controller = events[index], actors = {}, started = now, last_seen = now,
    }, Encounter)
    current.active = run
    if not run.controller.start(run, client, now) then
        run:reset()
        return false
    end
    return true
end

function M.enter(e)
    if not deathknell.arrival(e.self, "deathknell", {380, 1699, 2294, 746, 152}) then return end
    local dz = scope(e.self)
    if dz and initialize(dz) then eq.set_timer(tick_timer, 1000) end
end

function M.tick(e)
    if e.timer ~= tick_timer then return end
    local dz = scope(e.self)
    local current = dz and initialize(dz)
    local now = os.time()
    if not current or current.tick == now then return end
    current.tick = now
    local run = current.active
    if not run then
        local index = next_stage(dz)
        local entry = index and layout.events[index]
        if entry and entry.proximity then
            for client in eq.get_entity_list():GetClientList().entries do
                local member_dz = scope(client)
                if member_dz and member_dz:GetUUID() == current.uuid and client:GetHP() > 0
                    and near(client, entry.proximity, 30, 12) then
                    start(client, index)
                    break
                end
            end
        end
        return
    end
    if run.success then
        settle(run)
        return
    end
    if #run:players(run.info.presence, run.info.radius, run.info.height) > 0 then run.last_seen = now end
    if now >= run.last_seen + layout.abandon_seconds or not run.controller.tick(run, now) then run:reset() end
end

function M.say(e)
    local client = por.player(e.other)
    local dz = client and scope(client)
    local current = dz and initialize(dz)
    if not current then return end
    local words = por.words(e)
    if words == "destroy" and runtime.actor(e.self, layout.host) then
        start(client, 1)
    elseif words == "complete" and runtime.actor(e.self, layout.report_host)
        and current.active and current.active.stage == 3
        and current.active.waiting_report then
        current.active:finish()
    elseif words == "hail" and runtime.actor(e.self, layout.host)
        and next_stage(dz) == 1 and not current.active then
        e.self:Say("The watchers must not reach the dissonant bell. Tell me when you are ready to " ..
            eq.say_link("destroy") .. " the enforcers.")
    end
end

function M.door(e)
    local dz = scope(e.self)
    if not dz then return 1 end
    local id = e.door:GetDoorID()
    if layout.doors[id] ~= e.door:GetDoorDBID() then return 0 end
    local stage = next_stage(dz)
    if layout.vertigo_door[id] then
        if stage == 1 then return 1 end
        if id == 1 then
            if not near(e.self, vertigo_trapdoor, 28, 12) then return 1 end
            if stage == 2 and (not scene or not scene.active) and not start(e.self, 2) then return 1 end
            e.self:Message(MT.Yellow, "You clamber down through the trapdoor to the landing below.")
            e.self:MovePCInstance(381, dz:GetInstanceID(), vertigo_landing[1], vertigo_landing[2],
                vertigo_landing[3], e.self:GetHeading())
            return 1
        end
        if stage == 2 and (not scene or not scene.active) then
            return 1
        end
    elseif layout.lower_door[id] then
        if stage and stage < 3 then return 1 end
        if id == 3 then
            if not near(e.self, guests_trapdoor, 28, 12) then return 1 end
            e.self:Message(MT.Yellow, "You climb through the lower trapdoor to the rooms below.")
            e.self:MovePCInstance(381, dz:GetInstanceID(), guests_landing[1], guests_landing[2],
                guests_landing[3], e.self:GetHeading())
            return 1
        end
    end
    return 0
end

function M.damage(e)
    local run = scene and scene.active
    local target = e.self
    local id = target:GetID()
    local record = run and run.actors[id]
    if not record or target:GetEntityVariable(marker) ~= run.mark then return 0 end
    local owner = por.player(e.other)
    local dz = owner and scope(owner)
    if not dz or dz:GetUUID() ~= run.uuid or run.success then return -1 end
    if run.controller.damage then return run.controller.damage(run, e, record) end
    return 0
end

function M.death(e)
    local run = scene and scene.active
    local npc = e.killed
    local id = e.killed_entity_id
    local record = run and run.actors[id]
    if not record or not npc or not npc.valid or npc:GetNPCTypeID() ~= record.npc
        or npc:GetEntityVariable(marker) ~= run.mark or not runtime.corpse(e) then return end
    local client = por.player(e.other)
    local dz = client and scope(client)
    if not dz or dz:GetUUID() ~= run.uuid then return end
    run.actors[id] = nil
    run.controller.death(run, record, id, client)
end

function M.fetters(e)
    local run = scene and scene.active
    if not run or run.stage ~= 4 then return 0 end
    return run.controller.fetters(run, e, scope)
end

function M.fetters_effect(e)
    local run = scene and scene.active
    if not run or run.stage ~= 4 then return 0 end
    return run.controller.fetters_effect(run, e, scope)
end

return M
