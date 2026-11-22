local deathknell = require("rof2_por_deathknell")
local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")

local M = {}
local binding = deathknell.raids.sullon
local origin = {374,0,74,32,384}
local ward = {335.539,-109.965,-155.395}
local ward_timer = "por_sullon_ward"
local combat_timer = "por_sullon_combat"
local bosses = {
    [375940] = {spawn=304085513, name="Ghost, Guardian of Razorthorn", first=90, step=15},
    [375941] = {spawn=304085514, name="Snowtail, Guardian of Razorthorn", first=90, step=15},
    [375942] = {spawn=304085515, name="Sullon Zek", first=87, step=12},
}
local bones = {
    [372094] = {zone=372, spawn=104379, item=88102},
    [372028] = {zone=372, spawn=104004, item=88104},
    [374027] = {zone=374, spawn=106186, item=88103},
}
local states, pending = {}, nil

local function encounter_actor(npc)
    if not npc or not npc.valid then return end
    local npc_type = npc:GetNPCTypeID()
    local row = bosses[npc_type]
    if row and npc:GetSpawnPointID() == row.spawn then return row, npc_type end
end

local function clear_adds(state)
    for id in pairs(state.adds) do
        local add = runtime.owned(state.mark, id)
        if add then add:Depop(false) end
    end
end

local function eligible(dz, row, npc_type)
    if npc_type == 375942 then return not deathknell.won(dz, "sullon") end
    return not dz:HasLockout(row.name)
end

function M.enter(e)
    if not deathknell.arrival(e.self, "sullon", origin) then return end
    local dz = runtime.scope(e.self, binding)
    for npc, row in pairs(bosses) do
        if not eligible(dz, row, npc) then runtime.disable({npc=npc,spawn=row.spawn}, dz) end
    end
    eq.set_timer(ward_timer, 1000)
end

function M.ward(e)
    if e.timer ~= ward_timer then return end
    if pending and deathknell.commit(pending) then pending = nil end
    if e.self:CountItem(84164) == 0 then return end
    local dz = runtime.scope(e.self, binding)
    if not dz or not deathknell.won(dz, "sullon") then return end
    local dx, dy = e.self:GetX()-ward[1], e.self:GetY()-ward[2]
    if dx*dx + dy*dy <= 14*14 and math.abs(e.self:GetZ()-ward[3]) <= 16 then
        if e.self:TransformDeathknellSullonShard() then
            e.self:Message(MT.LightBlue, "Rallos Zek's ward shimmers within your shard.")
        end
    end
end

function M.door(e)
    local dz = runtime.scope(e.self, binding)
    if not dz then return 1 end
    local door = e.door:GetDoorDBID()
    if door == 304085513 and e.door:GetDoorID() == 1 and not deathknell.won(dz, "sullon") then
        e.self:Message(MT.Yellow, "The trapdoor is sealed by Sullon's rage.")
        return 1
    end
    if door == 304085549 and e.door:GetDoorID() == 37 then
        e.self:MovePC(origin[1], origin[2], origin[3], origin[4], origin[5])
        return 1
    end
    return 0
end

function M.combat(e)
    local npc = e.self
    local row, npc_type = encounter_actor(npc)
    if not row then return end
    local id = npc:GetID()
    local active = e.joined == true or tonumber(e.joined) == 1
    if not active then
        if states[id] then states[id].left = os.time() end
        return
    end
    local dz = runtime.scope(por.player(e.other), binding)
    if not dz or not eligible(dz, row, npc_type) then return end
    if states[id] then clear_adds(states[id]) end
    states[id] = {
        dz=dz, mark=dz:GetUUID() .. ":sullon:" .. id, adds={}, last_plague=os.time(),
        next_cast=os.time() + (npc_type == 375940 and 35 or 90), next_wave=row.first,
    }
    if npc_type == 375940 then npc:SpellFinished(6720, npc) end
    eq.set_next_hp_event(row.first)
    eq.set_timer(combat_timer, 1000, npc)
end

function M.hp(e)
    local npc = e.self
    local row, npc_type = encounter_actor(npc)
    local state = states[npc:GetID()]
    if not row or not state or tonumber(e.hp_event) ~= state.next_wave then return end
    local next_wave = state.next_wave - row.step
    state.next_wave = next_wave
    if next_wave > 0 then eq.set_next_hp_event(next_wave) end
    local wave = npc_type == 375942 and {375945,375945,375945,375945}
        or npc_type == 375941 and {375943,375943,375943}
        or {375943,375943,375943,375944,375944}
    local target = runtime.member_target(state.dz, binding, npc)
    for i, type_id in ipairs(wave) do
        local angle = i * math.pi * 2 / #wave
        local add = runtime.spawn_owned(type_id,
            runtime.pose(npc, math.cos(angle)*12, math.sin(angle)*12), state.mark)
        if add then
            state.adds[add:GetID()] = true
            if target and target.valid then add:AddToHateList(target, 1) end
        end
    end
end

local function random_cast(npc, dz, spells, count)
    local players = {}
    for client in eq.get_entity_list():GetClientList().entries do
        local owned = runtime.scope(client, binding)
        if owned and owned:GetUUID() == dz:GetUUID() and client:GetHP() > 0
            and npc:CalculateDistance(client:GetX(),client:GetY(),client:GetZ()) < 250 then
            players[#players + 1] = client
        end
    end
    for _ = 1, math.min(count, #players) do
        local target = table.remove(players, math.random(#players))
        npc:SpellFinished(spells[math.random(#spells)], target)
    end
end

function M.timer(e)
    if e.timer ~= combat_timer then return end
    local npc = e.self
    local row, npc_type = encounter_actor(npc)
    local state = states[npc:GetID()]
    if not row or not state then eq.stop_timer(combat_timer, npc); return end
    local now = os.time()
    if state.left then
        clear_adds(state)
        states[npc:GetID()] = nil
        eq.stop_timer(combat_timer, npc)
        return
    end
    if now >= state.next_cast then
        if npc_type == 375940 then
            random_cast(npc, state.dz, {7719}, 1)
            state.next_cast = now + 35
        elseif npc_type == 375941 then
            random_cast(npc, state.dz, {8150,8159,8266,8534}, math.random(3,5))
            state.next_cast = now + 90
        else
            random_cast(npc, state.dz, {5925,5926,6155}, 4)
            state.next_cast = now + 90
        end
    end
    if npc_type == 375942 and now >= state.last_plague + 180 then
        npc:SpellFinished(6598, npc)
        state.last_plague = now
    end
end

function M.death(e)
    local npc = e.killed
    local row, npc_type = encounter_actor(npc)
    if not row or not runtime.corpse(e) then return end
    -- Corpse creation clears the original NPC object ID before this callback.
    local id = e.killed_entity_id
    local dz, state = deathknell.terminal(e, "sullon"), states[id]
    if not dz or not state or state.dz:GetUUID() ~= dz:GetUUID() or not eligible(dz, row, npc_type) then return end
    if npc_type == 375942 then
        pending = deathknell.capture(dz, "sullon")
        if pending and deathknell.commit(pending) then pending = nil end
    else
        dz:AddLockout(row.name, 5 * 86400)
    end
    clear_adds(state)
    states[id] = nil
end

function M.bone_chest(e)
    local npc, killer = e.killed, por.player(e.other)
    if not killer or not npc or not npc.valid or not runtime.corpse(e) then return end
    local row = bones[npc:GetNPCTypeID()]
    if not row or not por.current(row.zone, 0, 0) or npc:GetSpawnPointID() ~= row.spawn then return end
    local chest = runtime.spawn_owned(371943, runtime.pose(npc, 6, 0), "por_bones:" .. row.spawn .. ":" .. npc:GetID())
    if chest then
        for _ = 1, 12 do chest:AddItem(row.item, 1, false) end
        eq.set_timer("por_bone_chest_timeout", 900000, chest)
    end
end

return M
