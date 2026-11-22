local layout = require("rof2_deathknell_tower_layout").choir
local M = {}
local boss_spells = {8970, 8971, 8972, 8573}

function M.start(run, client, now)
    local boss = run:spawn(layout.boss.npc, layout.boss.point, "ayonae")
    if not boss then return false end
    run.ayonae = boss:GetID()
    run.base_hp = boss:GetMaxHP()
    run.clicks, run.click_attempts = 0, {}
    run.group, run.boss_spell = 1, 1
    run.window_at, run.voices_at, run.cast_at = now + layout.fetters_interval, now + 60, now
    run:engage(boss, client)
    return true
end

local function capacity(run)
    return 1000 - run.clicks * 27
end

local function summon_voices(run, boss)
    local row = layout.parts[run.group]
    boss:Emote(row.emote)
    for index = 1, 4 + run.group % 2 do
        local point = row.points[index]
        local voice = run:spawn(row.npc, point, "voice")
        if not voice then return false end
        local target = run:target(run.info.presence, run.info.radius, run.info.height)
        run:engage(voice, target)
        if index % 2 == 0 then
            voice:SpellFinished(8574, voice)
        elseif target then
            voice:SpellFinished(8564, target)
        end
    end
    run.group = run.group % #layout.parts + 1
    return true
end

function M.tick(run, now)
    local boss = run:actor(run.ayonae)
    if not boss or now >= run.started + layout.maximum_seconds then return false end
    if now >= run.window_at then
        run.window_at = now + layout.fetters_interval
        run.window_end = now + layout.fetters_window
        run.click_attempts = {}
        boss:Emote("is momentarily distracted, exposing vulnerability.")
    end
    if now >= run.voices_at then
        run.voices_at = now + layout.voice_interval
        if not summon_voices(run, boss) then return false end
    end
    local left = capacity(run)
    if left <= 750 and now >= run.cast_at then
        local spell = boss_spells[run.boss_spell]
        run.boss_spell = run.boss_spell % #boss_spells + 1
        run.cast_at = now + (left <= 400 and 18 or 36)
        local player = run:target(run.info.presence, run.info.radius, run.info.height)
        if player then
            boss:SpellFinished(spell, spell == 8972 and player or boss)
        end
    end
    return true
end

function M.damage(run, e, record)
    if record.role == "ayonae" then return -1 end
    return 0
end

function M.death()
end

function M.fetters(run, e, scope)
    local owner, item = e.owner, e.self
    local dz = owner and scope(owner)
    local boss = run:actor(run.ayonae)
    if not dz or dz:GetUUID() ~= run.uuid or not boss or not item or not item.valid
        or item:GetID() ~= 52601 or owner:GetItemIDAt(e.slot_id) ~= 52601 then return 1 end
    local target = owner:GetTarget()
    if not target.valid or target:GetID() ~= boss:GetID() then return 1 end
    local now = os.time()
    if not run.window_end or now >= run.window_end then
        owner:Message(MT.Yellow, "Ayonae Ro is protected by her song.")
        return 1
    end
    run.click_attempts[owner:GetID()] = math.min(run.window_end, now + 3)
    return 0 -- Allow the real item cast, including its normal interruption and reuse.
end

function M.fetters_effect(run, e, scope)
    local victim = e.target
    local boss = run:actor(run.ayonae)
    if not boss or not victim.valid or victim:GetID() ~= boss:GetID() then return 0 end
    local caster = eq.get_entity_list():GetClientByID(e.caster_id)
    local dz = caster.valid and scope(caster)
    local now = os.time()
    local attempt = run.click_attempts[e.caster_id]
    run.click_attempts[e.caster_id] = nil
    if not dz or dz:GetUUID() ~= run.uuid or not attempt or now >= attempt
        or not run.window_end or now >= run.window_end then return 1 end
    run.window_end = nil
    run.clicks = run.clicks + 1
    local left = capacity(run)
    local cap = math.floor(run.base_hp * left / 1000)
    boss:ModifyNPCStat("max_hp", tostring(cap))
    boss:SetHP(cap)
    caster:Message(MT.LightBlue, string.format("The Divine Fetters bind Ayonae Ro. Her strength falls to %.1f%%.", left / 10))
    if left <= 190 then
        run:remove(run.ayonae)
        run:finish()
    end
    return 1
end

return M
