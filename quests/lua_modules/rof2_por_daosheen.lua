local deathknell = require("rof2_por_deathknell")
local runtime = require("rof2_dodh_raid_runtime")
local por = require("rof2_por_progression")

local M = {}
local binding = deathknell.raids.daosheen
local boss = {npc=371944, spawn=304085007}
local arkahn = {npc=371002, spawn=304085002}
local familiar = {npc=371003, spawn=304085003}
local timer = "por_daosheen_debuffs"

-- Crystal colors follow skylance.zon. Spirits spawn on the adjacent dry ledges.
local crystals = {
    [2] = {color="red", spell=8745, spirit={-327,-1,30,128}, step={0,4}},
    [5] = {color="green", spell=8746, spirit={-463,93,30,0}, step={4,0}},
    [6] = {color="blue", spell=8748, spirit={-442,-115,30,256}, step={0,4}},
    [7] = {color="yellow", spell=8747, spirit={-557,0,30,384}, step={0,4}},
}
local spirit_offsets = {0,-1,1}
local door_rows = {[2]=304085002,[5]=304085005,[6]=304085006,[7]=304085007}
local thresholds = {90,80,75,70,60,50,40,30,20,10,5}
local encounter, pending

local function members(dz)
    local players = {}
    for client in eq.get_entity_list():GetClientList().entries do
        local owned = runtime.scope(client, binding)
        if owned and owned:GetUUID() == dz:GetUUID() then players[#players + 1] = client end
    end
    return players
end

local function despawn_spirits(state)
    for id in pairs(state.spirits) do
        local add = runtime.owned(state.mark, id, 371940)
        if add then add:Depop(false) end
    end
end

function M.arkahn(e)
    local dz = runtime.scope(e.other, binding)
    if not dz or not runtime.actor(e.self, arkahn) then return end
    local words = por.words(e)
    if words == "hail" then
        e.self:Say("I am Arkahn, the gatekeeper. Are you headed to [" .. eq.say_link("Daosheen's study") .. "]?")
    elseif words == "daosheen's study" then
        e.self:Say("The study is open. Follow Firefeather up the ramp to the portal.")
        local guide = runtime.placed(familiar, dz)
        if guide then guide:MoveTo(0, 0, 246, 0, false) end
    end
end

local function cure(dz, spell)
    for _, client in ipairs(members(dz)) do
        client:BuffFadeBySpellID(spell)
    end
end

function M.door(e)
    local client, door = e.self, e.door
    local dz = runtime.scope(client, binding)
    if not dz then return 1 end
    local id = door:GetDoorID()
    if id == 3 and door:GetDoorDBID() == 304085003 then
        client:MovePCInstance(371, dz:GetInstanceID(), -464, 0, 5, 0)
        return 1
    elseif id == 4 and door:GetDoorDBID() == 304085004 then
        client:MovePCInstance(371, dz:GetInstanceID(), 0, -95, 2, 0)
        return 1
    end
    local crystal = crystals[id]
    if not crystal or door:GetDoorDBID() ~= door_rows[id] then return 0 end
    local state = encounter
    if not state or state.uuid ~= dz:GetUUID() then return 1 end
    local now = os.time()
    if now > state.last_click + 12 then state.color_sequence = "" end
    state.last_click = now
    local color = crystal.color
    state.color_sequence = (state.color_sequence .. color:sub(1,1)):sub(-4)
    if client:FindBuff(crystal.spell) then client:BuffFadeBySpellID(crystal.spell) end
    if state.color_sequence:sub(-2) == "yg" then cure(dz, 8750) end
    if state.color_sequence:sub(-2) == "rb" then cure(dz, 8751) end
    if state.color_sequence == "rybg" and now <= state.doom_until then
        cure(dz, 8749)
        state.doom_until = 0
    end
    return 1
end

function M.combat(e)
    local npc = e.self
    if not runtime.actor(npc, boss) then return end
    local engaged = e.joined == true or tonumber(e.joined) == 1
    if not engaged then
        if encounter then encounter.left = true end
        return
    end
    local dz = runtime.scope(por.player(e.other), binding)
    if not dz or deathknell.won(dz, "daosheen") then return end
    if encounter then despawn_spirits(encounter) end
    encounter = {
        dz=dz, uuid=dz:GetUUID(), mark=dz:GetUUID() .. ":daosheen:" .. npc:GetID(),
        next_threshold=1, spirits={}, color_sequence="", last_click=0, doom_until=0,
    }
    eq.set_next_hp_event(thresholds[1])
    eq.set_timer(timer, 16000, npc)
end

function M.hp(e)
    local state = encounter
    if not state or not runtime.in_instance(state.dz) or not runtime.actor(e.self, boss) then return end
    local percent = thresholds[state.next_threshold]
    if tonumber(e.hp_event) ~= percent then return end
    state.next_threshold = state.next_threshold + 1
    if thresholds[state.next_threshold] then eq.set_next_hp_event(thresholds[state.next_threshold]) end
    if percent <= 10 then
        state.doom_until = os.time() + 12
        state.color_sequence = ""
        e.self:SpellFinished(8749, e.self)
    end
    if percent == 75 or percent == 5 then return end
    local count = percent == 90 and 1 or (percent >= 50 and 2 or 3)
    local target = runtime.member_target(state.dz, binding, e.self)
    for _, crystal in pairs(crystals) do
        local p, step = crystal.spirit, crystal.step
        for index = 1, count do
            local offset = spirit_offsets[index]
            local spirit = runtime.spawn_owned(371940, {p[1]+step[1]*offset,p[2]+step[2]*offset,p[3],p[4]}, state.mark)
            if spirit then
                state.spirits[spirit:GetID()] = true
                if target and target.valid then spirit:AddToHateList(target, 1) end
            end
        end
    end
end

function M.timer(e)
    local state = encounter
    if e.timer ~= timer or not state or not runtime.in_instance(state.dz) then return end
    if state.left then
        despawn_spirits(state)
        encounter = nil
        eq.stop_timer(timer,e.self)
        return
    end
    local players = members(state.dz)
    if #players == 0 then return end
    local target = players[math.random(#players)]
    e.self:SpellFinished(8744 + math.random(4), target)
    if e.self:GetHPRatio() <= 75 then
        for i = #players, math.max(1, #players - 24), -1 do
            local j = math.random(i)
            players[i], players[j] = players[j], players[i]
            e.self:SpellFinished(8750, players[i])
        end
        e.self:SpellFinished(8751, e.self)
    end
end

function M.death(e)
    local npc, state = e.killed, encounter
    if not state or not runtime.actor(npc, boss) or not runtime.corpse(e) then return end
    local dz = deathknell.terminal(e, "daosheen")
    if not dz or dz:GetUUID() ~= state.uuid then return end
    pending = deathknell.capture(dz, "daosheen")
    encounter = nil
    M.settle()
end

function M.settle()
    if not pending then return end
    local credited = deathknell.commit(pending)
    if not credited then return end
    local dz = pending.dz
    pending = nil
    for _, id in ipairs(credited) do
        local client = deathknell.connected(id,dz,"daosheen")
        if client and client:CountItem(85622) == 0 then client:SummonItem(85622) end
    end
end

function M.enter(e)
    if not deathknell.arrival(e.self, "daosheen", {369,-836,-560,571,105}) then return end
    local dz = runtime.scope(e.self,binding)
    if deathknell.won(dz,"daosheen") then runtime.disable(boss,dz) end
    eq.set_timer("por_daosheen_settle", 1000)
end

return M
