local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")

local M = {}
local timer = "por_muse_event"
local states = {}
local definitions = {
    [380018]={spawn=107002, casts={8759,8760}, interval=20, add=380006},
    [380038]={spawn=107109, casts={9027,7337}, interval=25},
    [380040]={spawn=107123, casts={8707,7338,7339,7340,7341}, interval=18},
    [380046]={spawn=107160, casts={8753,8822,8826}, interval=23, wave={75,50,25}, add=380044},
    [380052]={spawn=107189, casts={8744}, interval=21, wave={90,80,70,60,50,40,30,20,10}, add=380043},
    [380059]={spawn=107289, casts={7335}, interval=30},
}
local audience = {380043,380045,380048,380050}

local function muse(npc)
    local id = npc:GetNPCTypeID()
    local row = definitions[id]
    if row and por.public(npc,380,id,row.spawn) then return row,id end
end

local function wipe(state)
    for id in pairs(state.adds) do
        local npc = runtime.owned(state.mark,id)
        if npc then npc:Depop(false) end
    end
    states[state.boss] = nil
end

local function create(npc)
    local id = npc:GetID()
    if states[id] then wipe(states[id]) end
    local now = os.time()
    local state = {
        boss=id, mark="por_muse:" .. id .. ":" .. tostring(now), next_cast=now+12,
        wave=1, adds={}, base_damage=npc:GetMaxDMG(), center=runtime.pose(npc),
        next_mulch=now+25, next_rage=now+18, damage_bonus=0,
    }
    states[id] = state
    eq.set_timer(timer,1000,npc)
    return state
end

local function spawn_add(state, kind, point, role, target)
    local add = runtime.spawn_owned(kind,point,state.mark)
    if not add then return end
    add:ClearItemList()
    add:ModifyNPCStat("max_hp",role == "echo" and "23000" or "46000")
    add:SetHP(add:GetMaxHP())
    add:ModifyNPCStat("min_hit","400")
    add:ModifyNPCStat("max_hit","1800")
    state.adds[add:GetID()] = {role=role,npc=kind}
    if target and target.valid then add:AddToHateList(target,1) end
    return add
end

function M.say(e)
    local _, npc_type = muse(e.self)
    if npc_type ~= 380052 then return end
    local words = por.words(e)
    if words == "hail" then
        e.self:Say("The audience awaits. Will you hear my [" .. eq.say_link("performance") .. "]?")
    elseif words == "performance" and not e.self:IsEngaged() then
        e.self:AddToHateList(e.other,1)
    end
end

function M.spawn(e)
    local _, npc_type = muse(e.self)
    if npc_type == 380052 then runtime.protect(e.self,true) end
    if npc_type == 380059 then eq.set_timer("por_valik_levitation",2000,e.self) end
end

function M.combat(e)
    local row, id = muse(e.self)
    if not row then return end
    local engaged = e.joined == true or tonumber(e.joined) == 1
    if not engaged then
        local state = states[e.self:GetID()]
        if state then state.left = true end
        return
    end
    if not por.player(e.other) then return end
    create(e.self)
    if row.wave then eq.set_next_hp_event(row.wave[1]) end
    if id == 380018 then e.self:SpellFinished(8761,e.self) end
end

function M.hp(e)
    local row, kind = muse(e.self)
    local state = states[e.self:GetID()]
    if not row or not row.wave or not state or tonumber(e.hp_event) ~= row.wave[state.wave] then return end
    state.wave = state.wave + 1
    if row.wave[state.wave] then eq.set_next_hp_event(row.wave[state.wave]) end
    if kind == 380046 then e.self:SpellFinished(8754,e.self) end
    local target = e.self:GetHateTop()
    for i=1,2 do
        local add = kind == 380052 and audience[math.random(#audience)] or row.add
        local role = kind == 380052 and "audience" or "guard"
        spawn_add(state,add,runtime.pose(e.self,9*i,6-i*4),role,target)
    end
end

local function reduce_movement(npc, center, max_distance)
    if npc:CalculateDistance(center[1],center[2],center[3]) > max_distance then
        npc:WipeHateList()
        npc:MoveTo(center[1],center[2],center[3],center[4],false)
        return true
    end
    return false
end

function M.timer(e)
    local npc = e.self
    local row, id = muse(npc)
    if e.timer == "por_valik_levitation" and id == 380059 then
        for client in eq.get_entity_list():GetClientList().entries do
            if npc:CalculateDistance(client) <= 90 then client:BuffFadeByEffect(57) end
        end
        return
    end
    if e.timer ~= timer then return end
    local state = states[npc:GetID()]
    if not row or not state then return end
    if state.left or (id == 380040 and reduce_movement(npc,state.center,100)) then
        npc:ModifyNPCStat("max_hit",tostring(state.base_damage))
        if id == 380052 then runtime.protect(npc,true) end
        wipe(state)
        eq.stop_timer(timer,npc)
        return
    end
    local now = os.time()
    if id == 380018 then
        if now >= state.next_mulch then
            state.next_mulch = now+26
            local spawn = runtime.pose(npc,math.random(-40,40),math.random(-40,40))
            local mulch = spawn_add(state,row.add,spawn,"mulch")
            if mulch then mulch:MoveTo(npc:GetX(),npc:GetY(),npc:GetZ(),0,false) end
        end
        for entity, data in pairs(state.adds) do
            local add = runtime.owned(state.mark,entity,data.npc)
            if add and data.role == "mulch" and add:CalculateDistance(npc) <= 6 then
                npc:SetHP(math.min(npc:GetMaxHP(),npc:GetHP()+math.floor(npc:GetMaxHP()*0.07)))
                state.adds[entity] = nil
                add:Depop(false)
            end
        end
    end
    if id == 380059 and now >= state.next_rage then
        state.next_rage = now+18
        state.damage_bonus = state.damage_bonus+1
        npc:ModifyNPCStat("max_hit",tostring(state.base_damage+state.damage_bonus*220))
    end
    if now >= state.next_cast then
        state.next_cast = now+row.interval
        local spell = row.casts[math.random(#row.casts)]
        local target = spell == 8753 and npc:GetHateTop() or npc
        if target and target.valid then npc:SpellFinished(spell,target) end
    end
end

function M.damage(e)
    local _, kind = muse(e.self)
    if kind ~= 380052 then return 0 end
    local state = states[e.self:GetID()]
    if not state then return -1 end
    return 0
end

function M.begin_performance(e)
    if por.words(e) == "performance" then
        local _, kind = muse(e.self)
        if kind == 380052 then runtime.protect(e.self,false) end
    end
    M.say(e)
end

function M.death(e)
    local dead = e.killed
    if not dead or not dead.valid or not runtime.corpse(e) then return end
    local id = e.killed_entity_id
    if states[id] then wipe(states[id]); return end
    for _, state in pairs(states) do
        local add = state.adds[id]
        if add and dead:GetNPCTypeID() == add.npc and dead:GetEntityVariable("dodh_raid:occurrence") == state.mark then
            state.adds[id] = nil
            local boss = eq.get_entity_list():GetNPCByID(state.boss)
            if not boss.valid then return end
            if add.role == "mulch" then
                state.damage_bonus = state.damage_bonus + 1
                boss:ModifyNPCStat("max_hit",tostring(state.base_damage+state.damage_bonus*240))
            elseif add.role == "audience" then
                for i=1,2 do
                    spawn_add(state,add.npc,runtime.pose(dead,4*i,-3*i),"echo",boss:GetHateTop())
                end
            end
            return
        end
    end
end

return M
