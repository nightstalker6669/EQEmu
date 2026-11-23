local route = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")

local M = {}
local ayna, fowl, haley, screechmaw = 406938, 406939, 406940, 406941
local assassin, cutthroat, murderer, krait = 406944, 406945, 406946, 406947
local stitch, pyrachnid, cinderweb, spiderling = 406949, 406950, 406951, 406952
local egg, ambersnout, goo, fragment, guardian = 406956, 406957, 406958, 406959, 406960
local sothgar, globe, lavascale, ashen, ashwing, ooze = 406973, 406974, 406975, 406976, 406977, 406978
local breakpoints = {75, 50, 25}

local function cast(s, npc, spell, target, delay, now)
    if npc and target and target.valid and now >= (s.cast_at[spell] or 0)
        and npc:CastSpell(spell, target:GetID()) then
        s.cast_at[spell] = now + delay
        return true
    end
    return false
end

local function cap(e, ratio)
    local floor = math.floor(e.self:GetMaxHP() * ratio / 100)
    local budget = e.self:GetHP() - floor
    if budget <= 0 then return -1 end
    if e.damage > budget then return budget end
    return 0
end

local function named(s, kind, point, target)
    local npc, id = s:spawn(kind, point, target)
    s.boss = id
    return npc
end

local function near(client, point, radius)
    return client:CalculateDistance(point[1], point[2], point[3]) <= radius
end

local function switch(s, packet)
    local door = eq.get_entity_list():GetDoorsByDoorID(packet)
    local expected = packet == 8 and s.spec.tunnel_outer or s.spec.tunnel_inner
    if door.valid and door:GetDoorDBID() == expected then return door end
end

local function start_ambush(s, client)
    local outer, inner = switch(s, 8), switch(s, 9)
    if not outer or not inner then return false end
    outer:ForceClose(client)
    inner:ForceClose(client)
    s.phase, s.next_wave = "ambush", s.started
    return true
end

local function start_stitch(s, client)
    local boss = named(s, stitch, s.event.point)
    if not boss then return false end
    s.spiders = {}
    for i, kind in ipairs({pyrachnid, cinderweb}) do
        local npc, id = s:spawn(kind, s.spec.spiders[i], client)
        if not npc then return false end
        s.spiders[id] = kind
    end
    s.phase = "spiders"
    return true
end

local function start_attendants(s)
    s.attendants = {}
    s.rank = {}
    s.next_rank = 1
    s.enraged_count = 0
    for index, point in ipairs(s.spec.attendants) do
        local npc, id = s:spawn(406961 + index, point)
        if not npc then return false end
        npc:SetPseudoRoot(true)
        s.attendants[index] = id
        s.rank[id] = index
    end
    s.phase = "attendants"
    return true
end

function M.launch(s, client)
    local id = s.event.id
    if id == 1 then
        return named(s, ayna, s.event.point) ~= nil
    elseif id == 2 then
        return start_ambush(s, client)
    elseif id == 3 then
        return start_stitch(s, client)
    elseif id == 4 then
        s.phase = "egg"
        return named(s, egg, s.event.point) ~= nil
    elseif id == 5 then
        return start_attendants(s)
    end
    local boss = named(s, sothgar, s.event.point)
    if not boss then return false end
    boss:SetAppearance(3)
    boss:SetPseudoRoot(true)
    s.break_index = 1
    return true
end

function M.say(s, e, members)
    if s.phase ~= "ready" or not near(e.other, runtime.pose(e.self), 45) then return end
    local words = route.words(e)
    if s.event.id == 1 and words == "fight" then
        if #members < 3 then e.self:Say("Bring more challengers to the cliff."); return end
        s.griffons, s.winds = {}, {}
        for i, kind in ipairs({fowl, haley}) do
            local npc, id = s:spawn(kind, s.spec.ayna_pets[i], e.other)
            local wind, wind_id = s:spawn(406941 + i, s.spec.ayna_pets[3 - i])
            if not npc or not wind then s:reset(); return end
            s.griffons[i], s.winds[i] = id, wind_id
        end
        s.phase = "griffons"
        s.next_wind, s.next_call, s.next_screech = os.time(), os.time() + 45, os.time() + 30
        e.self:AddToHateList(e.other, 1)
    elseif s.event.id == 6 and words == "awaken" then
        e.self:SetAppearance(0)
        e.self:AddToHateList(e.other, 1)
        s.phase = "dragon"
    end
end

function M.damage(s, e)
    local id, kind = e.self:GetID(), e.self:GetNPCTypeID()
    if s.event.id == 5 then
        if kind >= 406962 and kind <= 406966 then return cap(e, 10) end
        return s.enraged_count == 5 and 0 or -1
    end
    if id ~= s.boss then
        if kind == globe or kind == 406942 or kind == 406943 then return -1 end
        if s.event.id == 3 and s.phase == "spiders" and s.spiders[id] then return cap(e, 40) end
        return 0
    end
    if s.phase == "ready" or s.phase == "spiders" or s.phase == "brood" or s.phase == "pause" then
        return -1
    end
    if s.event.id == 6 and breakpoints[s.break_index] then return cap(e, breakpoints[s.break_index]) end
    return 0
end

local function tick_ayna(s, now, members)
    if s.phase ~= "griffons" then return end
    local boss = s:actor(s.boss)
    if not boss then return end
    cast(s, boss, 9448, members[math.random(#members)], 36, now)
    if now >= s.next_wind then
        s.next_wind = now + 6
        for index, id in ipairs(s.winds) do
            local wind = s:actor(id)
            if wind then
                for _, target in ipairs(members) do
                    if near(target, runtime.pose(wind), 28) then wind:SpellFinished(9445 + index, target) end
                end
                local point = s.spec.ayna_pets[(math.floor(now / 18) + index) % 2 + 1]
                wind:NavigateTo(point[1], point[2], point[3])
            end
        end
    end
    if now >= s.next_screech then
        s.next_screech = now + 50
        s:spawn(screechmaw, s.spec.ayna_pets[2], members[math.random(#members)])
    end
    if s.call_at and now >= s.call_at then
        for _, id in ipairs(s.griffons) do
            local pet = s:actor(id)
            if pet then pet:SpellFinished(9445, pet) end
        end
        s.call_at = nil
    elseif not s.call_at and now >= s.next_call then
        for _, id in ipairs(s.griffons) do
            local pet = s:actor(id)
            if pet then pet:Emote("draws in a deep breath.") end
        end
        s.call_at, s.next_call = now + 5, now + 58
    end
end

local function tick_ambush(s, now, members)
    if s.phase == "ambush" and now >= s.next_wave then
        local elapsed = now - s.started
        if elapsed >= 900 then
            local boss = named(s, krait, s.event.chest, members[1])
            if boss then s.phase = "krait" end
        else
            s.next_wave = now + 45
            local kind = elapsed < 300 and assassin or elapsed < 600 and cutthroat or murderer
            for _, point in ipairs(s.spec.tunnel_wave) do
                s:spawn(kind, point, members[math.random(#members)])
            end
        end
    end
    if s.phase == "ambush" and now >= (s.arrow_at or s.started + 35) then
        -- Stitch fires from the far side of the portcullis during the ambush.
        local archer = s:actor(s.archer)
        if not archer then
            local npc, id = s:spawn(406954, s.event.chest)
            archer, s.archer = npc, id
        end
        if archer then
            archer:SpellFinished(9223, members[math.random(#members)])
            s.arrow_at = now + 35
        end
    end
end

local function tick_stitch(s, now, members)
    local boss = s:actor(s.boss)
    if not boss then return end
    for id, kind in pairs(s.spiders) do
        local spider = s:actor(id)
        if spider then
            if s.phase == "spiders" and spider:GetHPRatio() <= 40 then
                s.phase = "stitch"
                boss:AddToHateList(members[1], 1)
                s.spider_at = now + 75
            end
            if kind == cinderweb then cast(s, spider, 9222, spider:GetHateTop(), 28, now) end
        end
    end
    if s.phase ~= "stitch" then return end
    cast(s, boss, 9223, members[math.random(#members)], 35, now)
    if now >= s.spider_at then
        s.spider_at = now + 75
        for _ = 1, math.random(2, 5) do
            s:spawn(spiderling, s.spec.spiders[math.random(2)], members[math.random(#members)])
        end
    end
end

local function tick_ambersnout(s, now, members)
    if s.phase ~= "brood" then return end
    local boss = s:actor(s.boss)
    if not boss then return end
    if now >= s.next_stop then
        local index = s.stop_index % #s.spec.ambersnout_stops + 1
        s.stop_index, s.next_stop = index, now + 32
        local point = s.spec.ambersnout_stops[index]
        boss:NavigateTo(point[1], point[2], point[3])
        local target = members[math.random(#members)]
        boss:SpellFinished(9166, boss)
        for n = 1, math.random(1, 3) do
            local kind = ({goo, fragment, guardian})[(n + index - 2) % 3 + 1]
            local add = s:spawn(kind, s.spec.ambersnout_stops[n], target)
            if add and kind == fragment then add:SpellFinished(9164, add) end
        end
    end
end

local function tick_attendants(s, now, members)
    for index, id in ipairs(s.attendants) do
        local npc = s:actor(id)
        if npc then
            for _, target in ipairs(members) do
                if near(target, s.spec.attendants[index], 48) and not target:FindBuff(9157 + index) then
                    npc:SpellFinished(9157 + index, target)
                end
            end
            if npc:GetNPCTypeID() <= 406966 and npc:GetHPRatio() <= 10 then
                local enraged, replacement = s:spawn(406966 + index, s.spec.attendants[index], npc:GetHateTop())
                if not enraged then s:reset(); return end
                enraged:SetPseudoRoot(true)
                s:forget(id)
                s.rank[id], s.rank[replacement] = nil, index
                s.attendants[index] = replacement
                s.enraged_count = s.enraged_count + 1
                if s.enraged_count == 5 then
                    local first = s:actor(s.attendants[s.next_rank])
                    if first then first:Emote("glows with a fierce light.") end
                end
            end
        end
    end
end

local function clear_adds(s)
    for id, kind in pairs(s.actors) do
        if kind >= globe and kind <= ooze then s:forget(id) end
    end
end

local function spawn_globes(s, now)
    s.globes = {}
    for index, point in ipairs(s.spec.sothgar_adds) do
        local hazard, id = s:spawn(globe, point)
        if hazard then
            local goal = s.spec.sothgar_adds[index % #s.spec.sothgar_adds + 1]
            hazard:NavigateTo(goal[1], goal[2], goal[3])
            s.globes[id] = now + 7 + 3 * index
        end
    end
    s.globe_cycle = now + 30
end

local function spawn_ooze(s, target)
    local hazard = s:spawn(ooze, s.spec.sothgar_adds[1], target)
    if not hazard then return end
    for id, kind in pairs(s.actors) do
        if kind >= lavascale and kind <= ashwing then
            local drake = s:actor(id)
            -- The retained damage shield is self-targeted in the spell catalog.
            if drake then drake:SpellFinished(9156, drake) end
        end
    end
end

local function spawn_drakes(s, members, with_ooze)
    local types = {lavascale, ashen, ashwing}
    for i, point in ipairs(s.spec.sothgar_adds) do
        s:spawn(types[i], point, members[math.random(#members)])
    end
    if with_ooze then spawn_ooze(s, members[1]) end
end

local function tick_sothgar(s, now, members)
    if s.phase == "ready" then return end
    local boss = s:actor(s.boss)
    if not boss then return end
    for _, client in ipairs(members) do
        if near(client, s.event.point, 145) then client:BuffFadeByEffect(57) end
    end
    if s.phase == "dragon" then
        local threshold = breakpoints[s.break_index]
        if threshold and boss:GetHP() <= math.floor(boss:GetMaxHP() * threshold / 100) then
            clear_adds(s)
            s.phase, s.resume_at = "pause", now + 135
            boss:SetDisableMelee(true)
            spawn_globes(s, now)
            return
        end
        cast(s, boss, 9154, boss:GetHateTop(), 31, now)
        cast(s, boss, 9155, boss, 45, now)
        if s.break_index == 4 then
            cast(s, boss, 5803, boss, 35, now)
            cast(s, boss, 9152, boss, 30, now)
        end
        if s.break_index >= 3 and not s.oozes and boss:GetHPRatio() <= 40 then
            s.oozes = true
            spawn_ooze(s, members[1])
        end
        return
    end
    for id, when in pairs(s.globes) do
        if now >= when then
            local hazard = s:actor(id)
            if hazard then hazard:SpellFinished(9153, hazard) end
            s:forget(id)
            s.globes[id] = nil
        end
    end
    if now >= s.resume_at then
        clear_adds(s)
        s.phase, s.break_index = "dragon", s.break_index + 1
        boss:SetDisableMelee(false)
        spawn_drakes(s, members, s.break_index == 4)
    elseif now >= s.globe_cycle then
        spawn_globes(s, now)
    end
end

local updates = {tick_ayna, tick_ambush, tick_stitch, tick_ambersnout, tick_attendants, tick_sothgar}

function M.tick(s, now, members)
    updates[s.event.id](s, now, members)
end

function M.death(s, e, client)
    local index, id, npc = s.event.id, e.killed_entity_id, e.killed
    local kind = npc:GetNPCTypeID()
    if index == 3 and s.spiders[id] then
        local effect, effect_id = s:spawn(406955, runtime.pose(npc))
        if effect then
            effect:SpellFinished(9225, effect)
            s:forget(effect_id)
        end
        s.spiders[id] = nil
        if not next(s.spiders) and s.boss_dead then s:win() end
        return
    end
    if index == 5 then
        local rank = s.rank[id]
        if not rank then return end
        if kind ~= 406966 + rank or rank ~= s.next_rank then s:reset(); return end
        s.next_rank = rank + 1
        if s.next_rank == 6 then s:win(); return end
        local next_one = s:actor(s.attendants[s.next_rank])
        if next_one and next_one:GetNPCTypeID() > 406966 then next_one:Emote("glows with a fierce light.") end
        return
    end
    if index == 4 and s.phase == "brood" and (kind == goo or kind == fragment or kind == guardian) then
        local boss = s:actor(s.boss)
        if not boss then return end
        s.brood_kills = s.brood_kills + 1
        if s.brood_kills < 12 then
            boss:SetHP(math.ceil(boss:GetMaxHP() * (12 - s.brood_kills) / 12))
            return
        end
        -- The last legitimate add death finishes Ambersnout and creates its snout corpse.
        s.phase = "finishing"
        boss:Damage(client, boss:GetMaxHP(), 65535, 28, false)
        return
    end
    if id ~= s.boss then return end
    if index == 4 and kind == egg then
        local boss = named(s, ambersnout, s.spec.ambersnout_stops[1], client)
        if boss then
            s.phase, s.stop_index, s.next_stop, s.brood_kills = "brood", 0, os.time(), 0
        else
            s:reset()
        end
        return
    end
    if index == 3 then
        s.boss_dead = true
        if not next(s.spiders) then s:win() end
    elseif index ~= 1 or runtime.corpse(e) then
        s:win()
    end
end

return M
