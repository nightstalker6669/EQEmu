local deathknell = require("rof2_por_deathknell")
local runtime = require("rof2_dodh_raid_runtime")
local por = require("rof2_por_progression")
local layout = require("rof2_por_suchun_layout")

local M = {}
local binding = deathknell.raids.suchun
local origin = {376,-554,727,225,384}
local wall = {npc=377944, spawn=304086025}
local portal_type, boss_type = 377943, 377940
local types = {blaze=377941, char=377942, suchun=boss_type}
local schools = {
    {skills={[Skill.Piercing1H]=true,[Skill.Piercing2H]=true}, resist=3, add=212007, spell=8883},
    {skills={[Skill.HandtoHand]=true}, resist=1, add=212040, spell=8884},
    {skills={[Skill.Slashing1H]=true,[Skill.Slashing2H]=true}, resist=1, add=212013, spell=8885},
    {skills={[Skill.Blunt1H]=true,[Skill.Blunt2H]=true}, resist=1, add=212036, spell=8886},
    {skills={[Skill.Archery]=true,[Skill.Throwing]=true}, resist=2, add=377013, spell=8887},
}
local tick_timer = "por_suchun_tick"
local state

local function spawn(npc_type, point)
    local npc = runtime.spawn_owned(npc_type, point, state.mark)
    if npc then state.owned[npc:GetID()] = npc_type end
    return npc
end

local function despawn_encounter()
    for id, npc_type in pairs(state.owned) do
        local actor = runtime.owned(state.mark, id, npc_type)
        if actor then actor:Depop(false) end
    end
end

local function nearby(actor, position, radius, height)
    local dx, dy = actor:GetX()-position[1], actor:GetY()-position[2]
    return dx*dx + dy*dy <= radius*radius and math.abs(actor:GetZ()-position[3]) <= height
end

local function timer_target(dz, center, radius)
    for client in eq.get_entity_list():GetClientList().entries do
        local scope = runtime.scope(client, binding)
        if scope and scope:GetUUID() == dz:GetUUID()
            and client:GetHP() > 0 and nearby(client, center, radius, 55) then
            return client
        end
    end
end

function M.arrive(e)
    if not deathknell.arrival(e.self, "suchun", origin) then return end
    local dz = runtime.scope(e.self, binding)
    if not state then
        state = {
            uuid=dz:GetUUID(), dz=dz, phase="approach", closed=true, tick=0, empty=0,
            mark=dz:GetUUID() .. ":suchun", owned={}, actors={}, portals={}, adds={}, lava={},
        }
    end
    if deathknell.won(dz, "suchun") then
        state.closed, state.phase = false, "victory"
        runtime.disable(wall, dz)
        e.self:TransformDeathknellSuchunShard()
    else
        local remains = runtime.placed(wall, dz)
        if remains and remains:GetEntityVariable("destructible_ruins") == "1" then state.closed = false end
    end
    eq.set_timer(tick_timer, 1000)
end

function M.wall_spawn(e)
    if not runtime.in_scope(binding) or not runtime.actor(e.self,wall) then return end
    e.self:SetEntityVariable("destructible_zone_actor", "DEST_TAK_ROOTWALL")
    e.self:SetEntityVariable("destructible_map_model", "DEST_TAK_ROOTWALL_M00")
    e.self:SetEntityVariable("destructible_retain_ruins", "1")
end

local function combat(npc, target)
    if npc and target then npc:AddToHateList(target, 1) end
end

local function spawn_portal(which)
    local npc = spawn(portal_type, layout.portals[which])
    if not npc then return false end
    state.portals[npc:GetID()] = {school=which, next_add=os.time()+18}
    return true
end

local function spawn_drakes(target, second)
    for _, role in ipairs({"blaze","char"}) do
        local npc = spawn(types[role], layout[role])
        if not npc then return false end
        state.actors[role] = npc:GetID()
        if second then npc:SetHP(math.floor(npc:GetMaxHP() * 0.50)) end
        combat(npc,target)
    end
    return true
end

local function start_final(target)
    local npc = spawn(boss_type, layout.suchun)
    if not npc then return false end
    state.actors.suchun = npc:GetID()
    npc:SetHP(math.floor(npc:GetMaxHP() * 0.60))
    npc:Shout("Foolish mortals! Your attempts to steal the Blood of Ro must be stopped!")
    combat(npc,target)
    return spawn_drakes(target,true) and spawn_portal(1)
end

local function start()
    state.phase, state.which, state.cast = "gateways", 1, os.time() + 18
    return spawn_portal(1)
end

local function reset()
    despawn_encounter()
    state.owned, state.actors, state.portals, state.adds, state.lava = {}, {}, {}, {}, {}
    state.phase, state.empty = "approach", 0
    state.mark = state.uuid .. ":suchun:" .. tostring(os.time())
end

local function advance(target)
    if state.phase == "gateways" then
        if next(state.portals) or next(state.adds) then return end
        if state.which < 5 then
            state.which = state.which + 1
            if not spawn_portal(state.which) then reset() end
        elseif spawn_drakes(target,false) then
            state.phase = "drakes"
        else
            reset()
        end
    elseif state.phase == "drakes" and not next(state.actors) then
        state.phase, state.which = "final", 1
        if not start_final(target) then reset() end
    elseif state.phase == "final" and not next(state.portals) then
        state.which = state.which % 5 + 1
        if not spawn_portal(state.which) then reset() end
    end
end

local function summons(target, now)
    for id, portal in pairs(state.portals) do
        if now >= portal.next_add then
            portal.next_add = now + 24
            local school = schools[portal.school]
            local p = layout.portals[portal.school]
            local add = spawn(school.add, {p[1]+3,p[2],p[3],p[4]})
            if add then
                add:ClearItemList()
                add:AddItem(39200, 3, false)
                if portal.school == 1 or portal.school == 3 or portal.school == 5 then
                    add:SetSpecialAbility(13, 1)
                end
                state.adds[add:GetID()] = portal.school
                combat(add,target)
            end
        end
    end
end

local function cast(now)
    if now < state.cast then return end
    state.cast = now + 20
    for role,id in pairs(state.actors) do
        local npc = runtime.owned(state.mark,id,types[role])
        if npc then
            local spell = role == "suchun" and (math.random(2)==1 and 8891 or 8893)
                or role == "blaze" and 8889 or 8888
            npc:SpellFinished(spell,npc)
            if role == "suchun" and spell == 8891 then
                local top = npc:GetHateTop()
                if top.valid then npc:SpellFinished(8890,top) end
            end
        end
    end
    for id, which in pairs(state.adds) do
        local school = schools[which]
        local add = runtime.owned(state.mark,id,school.add)
        local top = add and add:GetHateTop()
        if top and top.valid then add:SpellFinished(school.spell,top) end
    end
end

local function low_resources(dz, now)
    local id = state.actors.suchun
    local boss = id and runtime.owned(state.mark,id,boss_type)
    if not boss then return end
    for client in eq.get_entity_list():GetClientList().entries do
        local current = runtime.scope(client,binding)
        if current and current:GetUUID() == dz:GetUUID() and client:GetHP() > 0
            and nearby(client,layout.suchun,400,160) then
            local character_id = client:CharacterID()
            local mana = client:GetMaxMana() > 0 and client:GetManaRatio() or 100
            local resource = math.min(mana,client:GetEndurancePercent())
            local sent = state.lava[character_id]
            if resource >= 50 then
                state.lava[character_id] = nil
            elseif not sent then
                state.lava[character_id] = {grace=now+6, lethal=false}
                client:MovePCInstance(377,dz:GetInstanceID(),table.unpack(layout.lava))
            elseif resource < 20 and now >= sent.grace and not sent.lethal then
                sent.lethal = true
                boss:SpellFinished(982,client)
            end
        end
    end
end

function M.tick(e)
    if e.timer ~= tick_timer or not state or state.phase == "victory" then return end
    local dz = runtime.scope(e.self,binding)
    local now = os.time()
    if not dz or dz:GetUUID() ~= state.uuid or state.tick == now then return end
    state.tick = now
    if state.phase == "credit" then
        local members = deathknell.commit(state.pending)
        if not members then return end
        state.phase = "victory"
        despawn_encounter()
        local chest = runtime.spawn_owned(377945, layout.chest, state.mark .. ":chest")
        if chest then
            dz:SetLootEventByNPCTypeID(377945, deathknell.lockout("suchun"))
            eq.set_timer("por_suchun_chest_timeout", 900000, chest)
        end
        for _, id in ipairs(members) do
            local client = deathknell.connected(id,dz,"suchun")
            if client then client:TransformDeathknellSuchunShard() end
        end
        return
    end
    if state.closed then return end
    if state.phase == "approach" then
        local initiator = timer_target(dz, layout.approach, 44)
        if initiator and not start() then reset() end
        return
    end
    local target = timer_target(dz, layout.suchun, 210)
    if not target then
        if state.empty == 0 then state.empty = now end
        if now >= state.empty + 30 then reset() end
        return
    end
    state.empty = 0
    advance(target)
    summons(target, now)
    if state.phase ~= "approach" then cast(now) end
    if state.phase == "final" then low_resources(dz,now) end
end

local function school_hit(e, school)
    if e.is_damage_shield then return false end
    if e.spell_id == 65535 or e.spell_id <= 0 then return school.skills[e.skill_id] == true end
    local spell = eq.get_spell(e.spell_id)
    return spell.valid and spell:ResistType() == school.resist
end

function M.damage(e)
    if not state or not runtime.scope(por.player(e.other),binding) then return -1 end
    if runtime.actor(e.self, wall) then return state.closed and 0 or -1 end
    local id = e.self:GetID()
    local match = state.owned[id]
    if not match or not runtime.owned(state.mark,id,match) then return -1 end
    local portal = state.portals[id]
    local which = state.adds[id] or (portal and portal.school)
    if which and not school_hit(e,schools[which]) then return -1 end
    return 0
end

function M.death(e)
    local npc = e.killed
    if not state or not npc or not npc.valid then return end
    local dz = deathknell.terminal(e,"suchun")
    if not dz or dz:GetUUID() ~= state.uuid then return end
    if runtime.actor(npc,wall) and state.closed then
        state.closed = false
        return
    end
    local id = e.killed_entity_id
    local owned = state.owned[id]
    if not owned or npc:GetNPCTypeID() ~= owned
        or npc:GetEntityVariable("dodh_raid:occurrence") ~= state.mark then return end
    -- A gateway ignores hate and can die without producing a corpse.
    local gateway_without_corpse = owned == portal_type and state.portals[id]
        and (not e.corpse or not e.corpse.valid) and npc:GetID() == id and npc:GetHP() == 0
    if not gateway_without_corpse and not runtime.corpse(e) then return end
    state.owned[id], state.adds[id], state.portals[id] = nil,nil,nil
    for role, actor_id in pairs(state.actors) do
        if actor_id == id then state.actors[role] = nil; break end
    end
    if state.phase == "final" and not next(state.actors) then
        state.pending = deathknell.capture(dz,"suchun")
        if state.pending then state.phase = "credit" end
    end
end

return M
