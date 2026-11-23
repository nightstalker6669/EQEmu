local layout = require("rof2_tss_ashengate_layout")
local gameplay = require("rof2_tss_ashengate_encounters")
local route = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local read_one = require("rof2_dodh_scene_receipts").read_one

local M = {}
local scene
local initialized_uuid
local next_attempt = 0
local last_tick = 0
local timer = "tss_ashengate_raid"
local mark = "tss_ashengate:mark_source"
local lockout_seconds = 108 * 3600
local personal_tags = {}

local function close_to(client, point, radius)
    return client:CalculateDistance(point[1], point[2], point[3]) <= radius
end

local function present(spec, dz, point, radius)
    local members = {}
    for client in eq.get_entity_list():GetClientList().entries do
        local owned = runtime.scope(client, spec)
        if owned and owned:GetUUID() == dz:GetUUID() and client:GetHP() > 0
            and (not point or close_to(client, point, radius)) then
            members[#members + 1] = client
        end
    end
    return members
end

local Scene = {}
Scene.__index = Scene

function Scene:actor(id)
    return id and self.actors[id] and runtime.owned(self.uuid, id, self.actors[id])
end

function Scene:spawn(kind, point, target)
    local npc = runtime.spawn_owned(kind, point, self.uuid)
    if not npc then return end
    local id = npc:GetID()
    self.actors[id] = kind
    if kind == self.event.source then npc:SetEntityVariable(mark, self.uuid .. ":" .. self.event.id) end
    if target then npc:AddToHateList(target, 1) end
    return npc, id
end

function Scene:forget(id)
    local npc = self:actor(id)
    self.actors[id] = nil
    if npc then npc:Depop(false) end
end

function Scene:clear_except(kept)
    for id in pairs(self.actors) do
        if id ~= kept then self:forget(id) end
    end
end

function Scene:reset()
    self:clear_except()
    scene = nil
    next_attempt = os.time() + 15
end

function Scene:win()
    if self.winners then return end
    local winners = present(self.spec, self.dz, self.event.point, 260)
    if #winners == 0 then return end
    self.winners = {}
    for _, client in ipairs(winners) do self.winners[#self.winners + 1] = client:CharacterID() end
end

local function save_winners(s)
    local placeholders, args = {}, {s.event.id, s.uuid, s.dz:GetInstanceID(), s.spec.name, s.spec.version}
    local verify = {s.event.id, s.uuid, s.dz:GetInstanceID()}
    for _, id in ipairs(s.winners) do
        placeholders[#placeholders + 1] = "?"
        args[#args + 1] = id
        verify[#verify + 1] = id
    end
    local list = table.concat(placeholders, ",")
    local db, statement
    local ok = pcall(function()
        db = Database(Database.Default)
        statement = db:prepare([[
            INSERT INTO tss_ashengate_event_credit (dz_uuid,event_id,character_id,instance_id,earned_at)
            SELECT d.uuid,?,m.character_id,d.instance_id,UNIX_TIMESTAMP()
            FROM dynamic_zones d
            JOIN dynamic_zone_members m ON m.dynamic_zone_id=d.id
            JOIN instance_list i ON i.id=d.instance_id
            WHERE d.uuid=? AND d.instance_id=? AND d.name=? AND i.version=? AND i.zone=406
              AND m.character_id IN (]] .. list .. [[)
            ON DUPLICATE KEY UPDATE character_id=VALUES(character_id)
        ]])
        statement:execute(args)
    end)
    if statement then statement:close() end
    if db then db:close() end
    if not ok then return false end
    local row = read_one([[
        SELECT COUNT(*) FROM tss_ashengate_event_credit
        WHERE event_id=? AND dz_uuid=? AND instance_id=? AND character_id IN (]] .. list .. ")", verify)
    return row and tonumber(row[1]) == #s.winners
end

local function finish(s)
    if not s.committed then
        if not save_winners(s) then return end
        s.committed = true
    end
    if not s.dz:HasLockout(s.event.lockout) then
        s.dz:AddLockout(s.event.lockout, lockout_seconds)
        return
    end
    local result
    if s.event.chest then
        local _, id = s:spawn(s.event.source, s.event.chest)
        if not id then return end
        result = id
    end
    local activity = s.event.id == 3 and 4 or s.event.id == 6 and 0
    if activity then
        for _, id in ipairs(s.winners) do
            local client = eq.get_entity_list():GetClientByCharID(id)
            local dz = runtime.scope(client, s.spec)
            if dz and dz:GetUUID() == s.uuid then route.advance(client, s.spec.wing_task, activity) end
        end
    end
    s:clear_except(result)
    scene = nil
end

local function start(spec, dz, event, client)
    local s = setmetatable({
        spec = spec, dz = dz, event = event, uuid = dz:GetUUID(), actors = {}, phase = "ready",
        cast_at = {}, started = os.time(), last_present = os.time(),
    }, Scene)
    scene = s
    if not gameplay.launch(s, client) then s:reset() end
end

local function visit_diversion(client, spec)
    if not spec.diversion then return end
    for index, point in ipairs(spec.diversion) do
        if close_to(client, point, 28) then
            route.advance(client, spec.wing_task, index - 1)
            return
        end
    end
end

local function initialize_wing(spec, dz)
    local uuid = dz:GetUUID()
    if initialized_uuid == uuid then return end
    initialized_uuid = uuid
    local keep = {}
    for _, event in ipairs(spec.events) do
        dz:SetLootEventByNPCTypeID(event.source, event.lockout)
        if event.chest and dz:HasLockout(event.lockout) then
            keep[event.source] = uuid .. ":" .. event.id
        end
    end
    local first, last = 406938, 406955
    if spec.version == 21 then first, last = 406956, 406979 end
    -- A resumed worker has new entity IDs and no active encounter ledger.
    for npc in eq.get_entity_list():GetNPCList().entries do
        local kind = npc:GetNPCTypeID()
        if kind >= first and kind <= last and npc:GetEntityVariable("dodh_raid:occurrence") == uuid
            and (not keep[kind] or npc:GetEntityVariable(mark) ~= keep[kind]) then
            npc:Depop(false)
        end
    end
end

function M.enter(e, spec)
    local dz = runtime.scope(e.self, spec)
    if not dz then return end
    initialize_wing(spec, dz)
    eq.set_timer(timer, 1000)
end

function M.tick(e, spec)
    if e.timer ~= timer then return end
    local client, now = e.self, os.time()
    local dz = runtime.scope(client, spec)
    if not dz then return end
    visit_diversion(client, spec)
    if last_tick == now then return end
    last_tick = now
    if scene then
        if scene.uuid ~= dz:GetUUID() then return end
        if scene.winners then finish(scene); return end
        local members = present(spec, dz, scene.event.point, 240)
        if #members == 0 then
            if now - scene.last_present >= 45 then scene:reset() end
        else
            if now - scene.last_present >= 45 then scene:reset(); return end
            scene.last_present = now
            gameplay.tick(scene, now, members)
        end
        return
    end

    if now < next_attempt then return end
    for _, event in ipairs(spec.events) do
        local id = event.id
        local prerequisite = id ~= 3 and id ~= 6 or dz:HasLockout(spec.events[2].lockout)
        if id ~= 2 and prerequisite and not dz:HasLockout(event.lockout) then
            local radius = id == 3 and 90 or id == 4 and 75 or 130
            local nearby = present(spec, dz, event.point, radius)
            if id == 3 then
                for index = #nearby, 1, -1 do
                    if nearby[index]:GetX() <= -377 then table.remove(nearby, index) end
                end
            end
            if #nearby > 0 then start(spec, dz, event, nearby[1]); return end
        end
    end
end

function M.say(e, spec)
    if not scene or scene.spec ~= spec or scene.winners then return end
    local dz = runtime.scope(e.other, spec)
    if not dz or dz:GetUUID() ~= scene.uuid or e.self:GetID() ~= scene.boss then return end
    gameplay.say(scene, e, present(spec, dz, scene.event.point, 135))
end

function M.damage(e, spec)
    if not scene or scene.spec ~= spec or scene.winners then return -1 end
    local client = route.player(e.other)
    local dz = runtime.scope(client, spec)
    if not dz or dz:GetUUID() ~= scene.uuid or not scene:actor(e.self:GetID()) then return -1 end
    return gameplay.damage(scene, e, client)
end

function M.tag_diversion(e)
    local spec = layout.east
    local client = route.player(e.other)
    if not runtime.scope(client, spec) or not client:IsTaskActivityActive(spec.wing_task, 3) or e.damage <= 0 then return 0 end
    local npc = e.self
    local placement = npc:GetSpawnPointID()
    if placement < 304096000 or placement > 304096314 then return 0 end
    local id = npc:GetID()
    local tags = personal_tags[id]
    if not tags or npc:GetEntityVariable("tss_ashengate:diversion") ~= tostring(id) then
        tags = {}
        personal_tags[id] = tags
        npc:SetEntityVariable("tss_ashengate:diversion", tostring(id))
    end
    tags[client:CharacterID()] = true
    return 0
end

function M.death(e, spec)
    if not e.killed or not e.killed.valid or not e.killed:IsNPC() then return end
    local client = route.player(e.other)
    local dz = runtime.scope(client, spec)
    local id, npc = e.killed_entity_id, e.killed:CastToNPC()
    local tags = personal_tags[id]
    personal_tags[id] = nil
    if not dz then return end
    if spec.diversion and tags and npc:GetEntityVariable("tss_ashengate:diversion") == tostring(id) then
        for char_id in pairs(tags) do
            local member = eq.get_entity_list():GetClientByCharID(char_id)
            local own = runtime.scope(member, spec)
            if own and own:GetUUID() == dz:GetUUID() then route.advance(member, spec.wing_task, 3) end
        end
    end
    if not scene or scene.uuid ~= dz:GetUUID() or scene.winners
        or scene.actors[id] ~= npc:GetNPCTypeID()
        or npc:GetEntityVariable("dodh_raid:occurrence") ~= scene.uuid then return end
    scene.actors[id] = nil
    gameplay.death(scene, e, client)
end

function M.door(e, spec)
    if not e.door.valid then return 0 end
    local client, door = e.self, e.door
    local dz = runtime.scope(client, spec)
    if not dz then return 1 end
    local id = door:GetDoorDBID()
    if id == spec.blocked_gate or door:GetDoorID() == 3 then return 1 end
    if id == spec.tunnel_outer or id == spec.tunnel_inner then
        local event = spec.events[2]
        if dz:HasLockout(event.lockout) then return 0 end
        if scene and scene.event.id == 2 then return 1 end
        local x, y = client:GetX(), client:GetY()
        local in_tunnel = x > -685 and x < -377 and y > 1925 and y < 2125
            and math.abs(client:GetZ() + 187) < 18
        if not scene and os.time() >= next_attempt and in_tunnel then
            start(spec, dz, event, client)
            return 1
        end
        return id == spec.tunnel_inner and 1 or 0
    end
    if id == spec.exit and door:GetDoorID() == 13 and door:GetOpenType() == 58 then
        client:MovePC(405, -342, 3039, 375, 298)
        return 1
    end
    return 0
end

return M
