local layout = require("rof2_tss_vergalid_layout")
local route = require("rof2_por_progression")
local raid = require("rof2_tss_vergalid_raid")
local runtime = require("rof2_dodh_raid_runtime")

local M = {}
local binding, actors, spells = layout.ending, layout.actors, layout.spells
local events = {"Nurtha Frazzlesprocket", "Goru Uldrock", "Vergalid"}
local activities = {1, 2, 4}
local clockworks = {actors.gnomework, actors.arachnid, actors.vermin}
local waves = {actors.lost, actors.tormented, actors.angry, actors.ancient, actors.giant, actors.overlord, actors.wailing}
local hazard_mark = "tss_vergalid:hazard"
local timer = "tss_vergalid_end"
local encounter
local last_tick = 0
local cleaned_stale_actors = false

local function figure(id)
    if not encounter or not id then return end
    local kind = encounter.spawned[id]
    if kind then return runtime.owned(encounter.uuid, id, kind) end
end

local function exclude(npc)
    if npc then
        encounter.spawned[npc:GetID()] = nil
        npc:Depop(false)
    end
end

local function nearby(mob, point, radius)
    return mob:CalculateDistance(point[1], point[2], point[3]) <= radius
end

local function active_players(point, radius)
    return raid.alive_members(binding, encounter.uuid, point, radius)
end

local function choose(members)
    if #members > 0 then return members[math.random(#members)] end
end

local function remove_attempt()
    if not encounter then return end
    raid.clear(encounter)
    encounter = nil
end

local function start_scene(uuid, event, point)
    encounter = {
        uuid = uuid, event = event, point = point, spawned = {}, hazards = {}, protectors = {},
        completed = 0, cooldowns = {},
    }
end

local function armed_target(e)
    local client = route.player(e.other)
    if raid.member(client, encounter, binding) then return client end
end

local function begin_nurtha(now)
    local order = {clockworks[1], clockworks[2], clockworks[3]}
    for i = #order, 2, -1 do
        local j = math.random(i)
        order[i], order[j] = order[j], order[i]
    end
    encounter.machines = order
    encounter.next_machine = now
    encounter.next_contraption = now + 30
    encounter.phase = "machines"
end

local function tick_hazards(now)
    for id, hazard in pairs(encounter.hazards) do
        local npc = figure(id)
        if not npc or npc:GetEntityVariable("tss_vergalid:disarmed_by") ~= "" then
            encounter.hazards[id] = nil
            exclude(npc)
        else
            local target
            if hazard.target then
                local candidate = eq.get_entity_list():GetClientByCharID(hazard.target)
                if candidate.valid and candidate:GetHP() > 0 and raid.member(candidate, encounter, binding) then
                    target = candidate
                end
            end
            local contact = target and npc:CalculateDistance(target:GetX(), target:GetY(), target:GetZ()) <= 12
            if target and not contact then
                npc:NavigateTo(target:GetX(), target:GetY(), target:GetZ())
            end
            local triggered = not npc:IsLDoNTrapped()
            if triggered or contact or now >= hazard.detonates then
                if hazard.target then
                    if contact and not triggered then npc:SpellFinished(spells.blast, npc) end
                else
                    if not triggered then npc:SpellFinished(spells.web, npc) end
                    for index = 1, 4 do
                        raid.spawn(encounter, actors.spiderling, runtime.pose(npc, index * 3, 3),
                            choose(active_players(encounter.point, 140)))
                    end
                end
                exclude(npc)
                encounter.hazards[id] = nil
            end
        end
    end
end

local function hazard(kind, point, now, target)
    local npc = raid.spawn(encounter, kind, point, nil)
    if not npc then return end
    npc:SetEntityVariable(hazard_mark, encounter.uuid)
    npc:SetLDoNTrapType(1)
    npc:SetLDoNTrapSpellID(target and spells.blast or spells.web)
    npc:SetLDoNTrapped(true)
    encounter.hazards[npc:GetID()] = {target = target and target:CharacterID(), detonates = now + 25}
    if target then
        npc:Shout("Target selected for termination: " .. target:GetCleanName() .. ".")
        npc:SetRunning(true)
        npc:NavigateTo(target:GetX(), target:GetY(), target:GetZ())
    end
end

local function cast_when_due(npc, now, key, spell, target, delay)
    if (encounter.cooldowns[key] or 0) > now or not target or not target.valid then return end
    if npc:CastSpell(spell, target:GetID()) then encounter.cooldowns[key] = now + delay end
end

local function tick_nurtha(now, members)
    local nurse = figure(encounter.boss)
    if not nurse then return end
    if encounter.phase ~= "machines" then return end
    tick_hazards(now)
    if now >= encounter.next_contraption then
        encounter.next_contraption = now + 30
        for index = 1, 2 do hazard(actors.contraption, binding.points.contraptions[index], now, choose(members)) end
    end

    if encounter.next_machine and now >= encounter.next_machine then
        local count = encounter.completed
        local next_type = encounter.machines[count + 1]
        local machine = raid.spawn(encounter, next_type, binding.points.machines[count + 1], choose(members))
        if machine then
            encounter.machine = machine:GetID()
            encounter.next_machine = nil
            encounter.cooldowns = {}
        end
    end

    local machine = figure(encounter.machine)
    if not machine then return end
    local kind = machine:GetNPCTypeID()
    if kind == actors.gnomework and machine:GetHPRatio() <= 50 then
        cast_when_due(machine, now, "oil", spells.oil_change, machine, 50)
        cast_when_due(machine, now, "calamity", spells.calamity, machine, 32)
    elseif kind == actors.arachnid then
        cast_when_due(machine, now, "poison", spells.poison, machine, 40)
        if machine:GetHPRatio() <= 50 and now >= (encounter.cooldowns.old_chest or 0) then
            encounter.cooldowns.old_chest = now + 30
            hazard(actors.old_chest, binding.points.old_chest, now, nil)
        end
    elseif kind == actors.vermin then
        cast_when_due(machine, now, "rot", spells.flesh_rot, machine, 40)
        if machine:GetHPRatio() <= 50 and now >= (encounter.cooldowns.rats or 0) then
            encounter.cooldowns.rats = now + 60
            for index = 1, 3 do
                raid.spawn(encounter, actors.rat, runtime.pose(machine, 3 * index, 0), choose(members))
            end
        end
    end
end

local function tick_goru(now, members)
    local goru = figure(encounter.boss)
    if not goru or not encounter.next_wave then return end
    if encounter.phase == "boss" and not encounter.reinforced and
        goru:GetHP() <= math.floor(goru:GetMaxHP() / 2) then
        encounter.next_wave = now
        encounter.reinforced = true
    end
    if now >= encounter.next_wave then
        for index, point in ipairs(binding.points.undead) do
            local shift = (encounter.wave_number or 0) * 3
            local kind = waves[(index + shift - 1) % #waves + 1]
            raid.spawn(encounter, kind, point, choose(members))
        end
        encounter.wave_number = (encounter.wave_number or 0) + 1
        encounter.next_wave = now + 26
    end
    if encounter.phase == "boss" then
        cast_when_due(goru, now, "reign", spells.reign, goru:GetHateTop(), 36)
        cast_when_due(goru, now, "breath", spells.breath, goru, 42)
        cast_when_due(goru, now, "serric", spells.rot_of_serric, goru, 40)
    elseif encounter.completed >= 34 then
        encounter.phase = "boss"
        encounter.next_wave = now + 720
        goru:Shout("The restless dead can no longer protect me!")
        local target = choose(members)
        if target then goru:AddToHateList(target, 1) end
    end
end

local function current_event(client)
    for index, activity in ipairs(activities) do
        if client:IsTaskActivityActive(binding.task, activity) then return index end
    end
    if client:IsTaskActivityActive(binding.task, 3) then return 4 end
end

local function prepare(client, uuid)
    local index = current_event(client)
    local point = index and binding.points.event[index]
    if not point or not nearby(client, point, 155) then return end
    local status = index ~= 4 and client:GetVergalidEventLockout(events[index]) or "none"
    if status == "" or (index == 3 and status == "inherited") then return end
    if status == "inherited" or status == "earned" then
        raid.shared_step({client}, binding, activities[index])
        return
    end
    if encounter then return end
    start_scene(uuid, index, point)
    if index == 4 then
        for _, pad in ipairs(binding.points.protectors) do
            local npc = raid.spawn(encounter, actors.protector, pad)
            if not npc then remove_attempt(); return end
            encounter.protectors[npc:GetID()] = true
        end
        encounter.phase = "protectors"
        return
    end

    local kind = ({actors.nurtha, actors.goru, actors.dragon})[index]
    local npc = raid.spawn(encounter, kind, point)
    if not npc then remove_attempt(); return end
    encounter.boss = npc:GetID()
    encounter.phase = "waiting"
    if index >= 2 then npc:SetPseudoRoot(true) end
end

function M.say(e)
    local npc, client = e.self, e.other
    if not encounter or npc:GetID() ~= encounter.boss or not raid.member(client, encounter, binding) then return end
    if client:CalculateDistance(npc:GetX(), npc:GetY(), npc:GetZ()) > 45 then return end
    local words = route.words(e)
    local hail = words == "hail" or words == "hail, " .. npc:GetCleanName():lower()
    if encounter.event == 1 and encounter.phase == "waiting" and hail then
        npc:Say("You are too late to take back the flame.")
    elseif encounter.event == 2 and encounter.phase == "waiting" then
        if hail then
            npc:Say("You may face the souls I command if you will " .. eq.say_link("try") .. ".")
        elseif words == "try" or words == "we will try" then
            npc:Shout("Rise, forgotten dead!")
            encounter.phase = "undead"
            encounter.next_wave = os.time()
        end
    end
end

function M.damage(e)
    if not encounter or not armed_target(e) then return -1 end
    local id = e.self:GetID()
    if not figure(id) then return -1 end
    if id == encounter.boss then
        if encounter.event == 1 and encounter.phase == "waiting" then begin_nurtha(os.time()) end
        if encounter.phase ~= "boss" then return -1 end
        if encounter.event == 2 and not encounter.reinforced then return raid.cap_damage(e, 50) end
        return 0
    end
    if encounter.hazards[id] then return -1 end
    return 0
end

local function reward(event, client)
    local status = client:GetVergalidEventLockout(events[event])
    if status == "inherited" or status == "" then return false end
    if status ~= "earned" and not client:AddVergalidEventLockout(events[event]) then return false end
    return client:GetVergalidEventLockout(events[event]) == "earned"
end

local function finish_pending()
    if not encounter or not encounter.victory then return end
    local event = encounter.event
    local members = active_players(encounter.point, 220)
    local client = members[1]
    if not client then return end
    if event == 3 and not M.reward_dragon() then return end
    if event ~= 4 and not reward(event, client) then return end
    local activity = event == 4 and 3 or activities[event]
    if not raid.shared_step(members, binding, activity) then return end
    remove_attempt()
end

local function victory()
    encounter.victory = {}
    finish_pending()
end

function M.death(e)
    if not encounter or encounter.victory or not e.killed or not e.killed.valid then return end
    local id, kind = e.killed_entity_id, e.killed:GetNPCTypeID()
    if encounter.spawned[id] ~= kind or e.killed:GetEntityVariable("dodh_raid:occurrence") ~= encounter.uuid
        or not raid.death_owner(e, encounter, binding) then return end
    encounter.spawned[id] = nil
    if id == encounter.boss and encounter.phase == "boss" then
        if not runtime.corpse(e) then return end
        victory()
    elseif kind == actors.crystal and encounter.event == 3 and encounter.phase == "boss"
        and encounter.crystals[id] then
        encounter.crystals[id] = nil
        local uses = 0
        for _, member in ipairs(active_players(runtime.pose(e.killed), 45)) do
            local exile = member:FindBuff(spells.exile, encounter.boss)
            local breed = member:FindBuff(spells.breed, encounter.boss)
            if exile or breed then
                if exile then member:BuffFadeBySpellID(spells.exile) end
                if breed then member:BuffFadeBySpellID(spells.breed) end
                uses = uses + 1
                if uses == 6 then break end
            end
        end
    elseif id == encounter.machine and encounter.phase == "machines" then
        encounter.machine = nil
        encounter.completed = encounter.completed + 1
        if encounter.completed == 3 then
            local nurtha = figure(encounter.boss)
            if nurtha then
                nurtha:SetHP(math.max(1, math.floor(nurtha:GetMaxHP() * .02)))
                encounter.phase = "boss"
                local target = choose(active_players(encounter.point, 155))
                if target then nurtha:AddToHateList(target, 1) end
            end
        else
            encounter.next_machine = os.time() + (encounter.completed == 1 and 30 or 60)
        end
    elseif encounter.event == 2 and encounter.phase == "undead" and id ~= encounter.boss then
        encounter.completed = encounter.completed + 1
    elseif encounter.phase == "protectors" and encounter.protectors[id] then
        encounter.protectors[id] = nil
        if not next(encounter.protectors) then victory() end
    end
end

function M.arrive(e)
    if raid.arrive(e, binding) then eq.set_timer(timer, 1000) end
end

function M.tick(e)
    local now = os.time()
    if e.timer ~= timer or now == last_tick then return end
    local uuid = raid.active(e.self, binding)
    if not uuid then return end
    last_tick = now
    if encounter and encounter.uuid ~= uuid then return end
    if not cleaned_stale_actors then
        -- Stock NPC snapshots can retain unfinished scripted actors without
        -- their Lua occurrence marks. Boss receipts and corpses remain intact.
        for npc in eq.get_entity_list():GetNPCList().entries do
            local kind = npc:GetNPCTypeID()
            if kind >= actors.nurtha and kind <= actors.corruption
                and not npc:IsCorpse() then npc:Depop(false) end
        end
        cleaned_stale_actors = true
    end
    if not encounter then
        for _, client in ipairs(raid.alive_members(binding, uuid)) do
            prepare(client, uuid)
            if encounter then break end
        end
        return
    end
    if encounter.victory then finish_pending(); return end
    local members = active_players(encounter.point, 180)
    if #members == 0 then
        encounter.abandoned = encounter.abandoned or now
        if now - encounter.abandoned >= 60 then remove_attempt() end
        return
    end
    encounter.abandoned = nil
    if encounter.event == 1 then tick_nurtha(now, members) end
    if encounter.event == 2 then
        tick_goru(now, members)
        for id, kind in pairs(encounter.spawned) do
            if kind == actors.wailing then
                local soul = figure(id)
                if soul then cast_when_due(soul, now, id, spells.rot_of_serric, soul, 27) end
            end
        end
    end
    if encounter.event == 3 then M.tick_dragon(now, members) end
end

function M.tick_dragon(now, members)
    local boss = figure(encounter.boss)
    if not boss then return end
    if encounter.phase == "waiting" then
        local nearby_members = active_players(binding.points.dragon_trigger, 105)
        if #nearby_members >= 6 then
            boss:Shout("The crystals tremble as Vergalid awakens.")
            encounter.phase = "awakening"
            encounter.awakens = now + 10
        end
        return
    end
    if encounter.phase == "awakening" then
        if now < encounter.awakens then return end
        local pads = binding.points.crystals
        encounter.crystals = {}
        for _, pad in ipairs(pads) do
            local crystal = raid.spawn(encounter, actors.crystal, pad)
            if crystal then encounter.crystals[crystal:GetID()] = 6 end
        end
        if not next(encounter.crystals) then remove_attempt(); return end
        encounter.phase = "boss"
        encounter.exile = now + 40
        encounter.pit_exiles = {}
        encounter.cooldowns.roar = now + 60
        encounter.cooldowns.siphon = now + 36
        boss:SetEntityVariable("tss_vergalid:dragon", encounter.uuid)
        boss:AddToHateList(choose(members), 1)
    end

    if not encounter.breed and boss:CastSpell(spells.breed, boss:GetID()) then
        encounter.breed = true
    end
    cast_when_due(boss, now, "roar", spells.roar, boss, 60)
    cast_when_due(boss, now, "siphon", spells.siphon, boss:GetHateTop(), 36)

    if now >= encounter.exile then
        encounter.exile = now + 40
        local targets = active_players(encounter.point, 170)
        for index = 1, math.min(6, #targets) do
            local pick = math.random(index, #targets)
            targets[index], targets[pick] = targets[pick], targets[index]
            targets[index]:ReceiveVergalidDoomingExile(boss)
        end
    end

    local pit = binding.points.pit
    for char_id in pairs(encounter.pit_exiles) do
        local client = eq.get_entity_list():GetClientByCharID(char_id)
        if not client.valid or client:GetHP() <= 0 or not raid.member(client, encounter, binding) then
            encounter.pit_exiles[char_id] = nil
        elseif client:GetZ() <= pit.bottom and
            (client:GetX() - pit.x)^2 + (client:GetY() - pit.y)^2 <= pit.radius^2 then
            encounter.pit_exiles[char_id] = nil
            boss:SpellFinished(982, client)
        end
    end
end

function M.fade(e)
    if not encounter or encounter.event ~= 3 or encounter.phase ~= "boss" or
        e.caster_id ~= encounter.boss or e.tics_remaining > 0 then return end
    local client = route.player(e.target)
    if not raid.member(client, encounter, binding) or client:GetHP() <= 0 then return end
    if e.spell_id == spells.exile then
        local p = binding.points.pit
        encounter.pit_exiles[client:CharacterID()] = true
        client:MovePCInstance(404, eq.get_zone():GetInstanceID(), p.x, p.y, p.z, p.heading)
    elseif e.spell_id == spells.breed then
        raid.spawn(encounter, actors.corruption, runtime.pose(client, 4, 0), client)
    end
end

function M.reward_dragon()
    if encounter.victory.chest then return true end
    local existing = eq.get_entity_list():GetNPCByNPCTypeID(actors.dragon_chest)
    if existing.valid then
        encounter.victory.chest = existing:GetID()
        return true
    end
    local chest = runtime.spawn_owned(actors.dragon_chest, binding.points.dragon_chest, encounter.uuid)
    if not chest then return false end
    encounter.victory.chest = chest:GetID()
    return true
end

function M.loot(e)
    if not e.corpse.valid or not e.corpse:IsNPCCorpse() then return 0 end
    local kind = e.corpse:GetNPCTypeID()
    local event = kind == actors.nurtha and events[1] or kind == actors.goru and events[2]
        or kind == actors.dragon_chest and events[3]
    if not event then return 0 end
    return (e.other.valid and e.other:HasVergalidEventLoot(event)) and 0 or 1
end

return M
