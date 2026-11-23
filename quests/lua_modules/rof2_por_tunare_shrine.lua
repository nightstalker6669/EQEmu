local unpack_args = table.unpack or unpack
local layout = require("rof2_por_tunare_layout")
local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local public = require("rof2_por_tunare_public")
local read_one = require("rof2_dodh_scene_receipts").read_one

local M = {}
local survey, question, raid = layout.stages[1], layout.stages[2], layout.stages[4]
local nalderin, brazlin, guardian, drekish = 379940, 379941, 379942, 379946
local core_types = {379943, 379944, 379945}
local touch = 982
local encounter = {}
local initialized = false

local function stage_here()
    for _, stage in ipairs({survey, question, raid}) do
        if por.current(layout.shrine, stage.version) and eq.get_zone():GetInstanceID() > 0 then
            return stage
        end
    end
end

local function member(client, stage)
    return client and client.valid and stage and por.inside(client, stage.id, layout.shrine, stage.version)
end

local function nearby(a, x, y, z, range, height)
    local dx, dy = a:GetX() - x, a:GetY() - y
    return dx*dx + dy*dy <= range*range and math.abs(a:GetZ() - z) <= height
end

local function front_member(e, stage)
    local client = e.other
    return member(client, stage)
        and nearby(client, e.self:GetX(), e.self:GetY(), e.self:GetZ(), 45, 18)
end

function M.enter(e)
    if stage_here() then eq.set_timer("tunare_shrine", 1000) end
end

local function recover_unfinished(stage)
    if initialized then return end
    initialized = true
    for npc in eq.get_entity_list():GetNPCList().entries do
        local id = npc:GetNPCTypeID()
        if stage == raid then
            if id >= guardian and id <= drekish and npc:GetSpawnPointID() == 0 then
                npc:Depop(false)
            elseif id == brazlin then
                npc:GMMove(unpack_args(layout.brazlin_pad))
            end
        elseif stage == question and id == nalderin then
            npc:WipeHateList()
            npc:BuffFadeBySpellID(layout.nalderin_aura)
            npc:GMMove(unpack_args(layout.nalderin_floors[1]))
            npc:SetHP(npc:GetMaxHP())
        end
    end
end

function M.click(e)
    local stage = stage_here()
    if not stage or not e.self.valid or not e.door.valid then return 0 end
    local packet, db = e.door:GetDoorID(), e.door:GetDoorDBID()
    if packet == 1 and db == stage.entry then
        if nearby(e.self, 0.696, -9.285, 2.475, 30, 16) then
            e.self:MovePC(layout.forest, unpack_args(layout.forest_return))
        end
        return 1
    end
    if stage == raid and packet == 2 and db == raid.upper and not member(e.self, stage) then
        local retained = public.retained_raid_instance(e.self)
        if retained ~= eq.get_zone():GetInstanceID() then return 1 end
    end
    return 0
end

local function survey_ready(client)
    for i = 2, 5 do
        if client:IsTaskActivityActive(survey.id, i) then return true end
    end
    return false
end

function M.shrine_damage(e)
    local stage, client = stage_here(), por.player(e.other)
    if not member(client, stage) then return -1 end
    if stage == survey and not survey_ready(client) then
        e.self:WipeHateList()
        return -1
    end
    return 0
end

local prompts = {
    {hint = "failed", middle = "Yes, they failed to retrieve this [information].",
        finish = "what information", alt = "information"},
    {hint = "figurine", middle = "You don't [want to know] where the figurine came from.",
        finish = "i want to know", alt = "want to know"},
    {hint = "key", finish = "what key"},
    {hint = "tell you everything", finish = "tell you everything"},
    {hint = "greater power", finish = "what greater power"},
}

function M.nalderin_say(e)
    if not front_member(e, question) or e.self:GetNPCTypeID() ~= nalderin then return end
    recover_unfinished(question)
    if not front_member(e, question) then return end
    local words = por.words(e)
    local round = encounter.priest_round or 1
    local prompt = prompts[round]
    if encounter.priest_fighting then return end
    if words == "hail" then
        por.advance(e.other, question.id, 0)
        e.self:Say("You think I will tell you about " .. eq.say_link(prompt.hint) .. "?")
        return
    end
    if prompt.middle and (words == prompt.hint or words == "who " .. prompt.hint
        or words == "what " .. prompt.hint) then
        e.self:Say(prompt.middle)
        return
    end
    if words ~= prompt.finish and words ~= prompt.alt and
        (prompt.middle or words ~= prompt.hint) then return end
    if e.other:IsTaskActivityActive(question.id, 0) then return end
    encounter.priest_fighting = true
    e.self:Say("I will die with this information if I must!")
    e.self:CastSpell(layout.nalderin_aura, e.self:GetID())
    e.self:AddToHateList(e.other, 1)
end

local function capped_damage(e, percent)
    local boundary = math.floor(e.self:GetMaxHP() * percent / 100)
    local remaining = e.self:GetHP() - boundary
    if remaining <= 0 then return -1 end
    if e.damage >= remaining then return remaining end
    return 0
end

function M.nalderin_damage(e)
    if not member(por.player(e.other), question) or not encounter.priest_fighting then return -1 end
    local round = encounter.priest_round or 1
    if round == 5 then return 0 end
    return capped_damage(e, 100 - 20 * round)
end

function M.nalderin_tick(e)
    if e.timer ~= "tunare_nalderin" or not encounter.priest_fighting then return end
    local round = encounter.priest_round or 1
    if round == 5 or e.self:GetHP() > math.floor(e.self:GetMaxHP() * (100 - 20*round) / 100) then
        return
    end
    e.self:Shout("Your pitiful efforts will never get me to talk!")
    e.self:WipeHateList()
    e.self:BuffFadeBySpellID(layout.nalderin_aura)
    e.self:GMMove(unpack_args(layout.nalderin_floors[round + 1]))
    encounter.priest_round, encounter.priest_fighting = round + 1, false
end

local function spawn(type_id, point)
    return eq.spawn2(type_id, 0, 0, unpack_args(point))
end

local function mission_uuid(client)
    local row = read_one([[
        SELECT d.uuid
          FROM shared_task_dynamic_zones sd
          JOIN dynamic_zones d ON d.id=sd.dynamic_zone_id
         WHERE sd.shared_task_id=? AND d.instance_id=?
    ]], {client:GetSharedTaskIdentity(), eq.get_zone():GetInstanceID()})
    return row and row[1]
end

local function clear_cores()
    for id in pairs(encounter.cores) do
        local add = eq.get_entity_list():GetNPCByID(id)
        if add.valid then add:Depop(false) end
    end
    encounter.cores = {}
end

local function lap_move(priest)
    local point = layout.brazlin_lap[encounter.lap]
    priest:MoveTo(point[1], point[2], layout.brazlin_pad[3], 0, false)
end

function M.brazlin_say(e)
    if not front_member(e, raid) or e.self:GetNPCTypeID() ~= brazlin then return end
    recover_unfinished(raid)
    if not front_member(e, raid) then return end
    local words = por.words(e)
    if words == "hail" then
        por.advance(e.other, raid.id, 0)
        e.self:Say("You fools think that " .. eq.say_link("you can stop the corruption") .. "?")
        return
    end
    if words ~= "we can stop the corruption" and words ~= "you can stop the corruption" then return end
    if encounter.guardian or (not e.other:IsTaskActivityActive(raid.id, 1)
        and not e.other:IsTaskActivityActive(raid.id, 2)) then return end
    local uuid = mission_uuid(e.other)
    if not uuid then return end
    local boss = spawn(guardian, layout.guardian_pad)
    if not boss.valid then return end
    encounter.guardian, encounter.brazlin = boss:GetID(), e.self:GetID()
    boss:SetEntityVariable("por_tunare:guardian", uuid)
    encounter.guardian_wave, encounter.cores, encounter.lap = 0, {}, 1
    encounter.cleared_this_lap = false
    encounter.wave_open, encounter.misses, encounter.idle_since = false, 0, nil
    encounter.root_at, encounter.corruption_at = os.time() + 20, os.time() + 35
    lap_move(e.self)
    e.self:Say("Guardian! Show these pathetic souls what pain is.")
    boss:AddToHateList(e.other, 1)
    por.advance(e.other, raid.id, 1)
end

function M.guardian_damage(e)
    if not member(por.player(e.other), raid) or e.self:GetID() ~= encounter.guardian then return -1 end
    if encounter.wave_open then return -1 end
    if encounter.guardian_wave == 4 then return 0 end
    return capped_damage(e, 80 - 20 * encounter.guardian_wave)
end

local function lap_finished(boss, priest)
    local healed = encounter.wave_open and next(encounter.cores)
    local late = healed or (encounter.guardian_wave < 4 and not encounter.wave_open)
    if late then
        encounter.misses = encounter.misses + 1
        if healed then
            priest:Shout("Brazlin channels energy from the core golems and heals his guardian.")
            boss:SetHP(math.min(boss:GetMaxHP(), boss:GetHP() + math.floor(boss:GetMaxHP() / 5)))
            clear_cores()
            encounter.guardian_wave = encounter.guardian_wave - 1
        end
        if encounter.misses >= 2 then
            local enforcer = encounter.enforcer and eq.get_entity_list():GetNPCByID(encounter.enforcer)
            if not enforcer or not enforcer.valid then
                enforcer = spawn(drekish, layout.drekish_pad)
                if enforcer.valid then encounter.enforcer = enforcer:GetID() end
            end
            if enforcer.valid then
                local targets = {}
                for client in eq.get_entity_list():GetClientList().entries do
                    if member(client, raid) and nearby(client, 0, 83, 282.487, 110, 16) then
                        targets[#targets+1] = client
                    end
                end
                if #targets > 0 then enforcer:CastSpell(touch, targets[math.random(#targets)]:GetID()) end
            end
        end
    else
        encounter.misses = 0
    end
    encounter.wave_open = false
end

local function reset_guardian(boss, priest)
    clear_cores()
    if encounter.enforcer then
        local enforcer = eq.get_entity_list():GetNPCByID(encounter.enforcer)
        if enforcer.valid then enforcer:Depop(false) end
    end
    priest:GMMove(unpack_args(layout.brazlin_pad))
    boss:Depop(false)
    encounter = {}
end

function M.guardian_tick(e)
    if e.timer ~= "tunare_guardian" or e.self:GetID() ~= encounter.guardian then return end
    local boss, wave = e.self, encounter.guardian_wave
    local priest = eq.get_entity_list():GetNPCByID(encounter.brazlin)
    if not priest.valid then return end

    local target = por.player(boss:GetHateTop())
    if not member(target, raid) then
        encounter.idle_since = encounter.idle_since or os.time()
        if os.time() - encounter.idle_since >= 45 then reset_guardian(boss, priest) end
        return
    end
    encounter.idle_since = nil
    local now = os.time()
    if now >= encounter.corruption_at then
        boss:CastSpell(7342, target:GetID())
        encounter.corruption_at = now + 45
    elseif now >= encounter.root_at then
        boss:CastSpell(8617, target:GetID())
        encounter.root_at = now + 30
    end

    if encounter.wave_open and not next(encounter.cores) then
        encounter.wave_open = false
        encounter.cleared_this_lap = true
    end
    if not encounter.wave_open and wave < 4
        and boss:GetHP() <= math.floor(boss:GetMaxHP() * (80 - 20*wave) / 100) then
        boss:Shout("The Guardian summons elementals of protection and commands them to attack!")
        for i, point in ipairs(layout.core_pads) do
            local type_id = core_types[(i-1) % #core_types + 1]
            local add = spawn(type_id, point)
            if add.valid then
                encounter.cores[add:GetID()] = type_id
                add:AddToHateList(target, 1)
            end
        end
        encounter.guardian_wave, encounter.wave_open = wave + 1, true
    end

    local point = layout.brazlin_lap[encounter.lap]
    if nearby(priest, point[1], point[2], layout.brazlin_pad[3], 4, 12) then
        encounter.lap = encounter.lap + 1
        if encounter.lap > #layout.brazlin_lap then
            if encounter.wave_open or not encounter.cleared_this_lap then lap_finished(boss, priest)
            else encounter.misses = 0 end
            encounter.cleared_this_lap = false
            encounter.lap = 1
        end
        lap_move(priest)
    end
end

function M.death(e)
    local stage, client = stage_here(), por.player(e.other)
    if not member(client, stage) or not e.killed or not e.killed.valid or not runtime.corpse(e) then return end
    local type_id, id = e.killed:GetNPCTypeID(), e.killed_entity_id
    if stage == question and type_id == nalderin and encounter.priest_round == 5
        and encounter.priest_fighting then
        por.advance(client, question.id, 1)
    elseif stage == raid and encounter.cores and encounter.cores[id] == type_id then
        encounter.cores[id] = nil
    elseif stage == raid and type_id == guardian and id == encounter.guardian
        and encounter.guardian_wave == 4 and not encounter.wave_open then
        local priest = eq.get_entity_list():GetNPCByID(encounter.brazlin)
        if priest.valid then
            priest:Shout("You weaklings will never stop the corruption!")
            priest:Depop(false)
        end
        if encounter.enforcer then
            local enforcer = eq.get_entity_list():GetNPCByID(encounter.enforcer)
            if enforcer.valid then enforcer:Depop(false) end
        end
        por.advance(client, raid.id, 2)
    end
end

function M.spawn(e)
    local stage, npc = stage_here(), e.other
    if not stage or not npc.valid then return end
    local type_id = npc:GetNPCTypeID()
    if type_id < 379000 or type_id > 379013 then return end
    if npc:GetSpawnPointID() == 0 then return end

    -- Retained geometry and NPC appearances have placeholder combat values.
    npc:ModifyNPCStat("max_hp", stage == raid and "105000" or "56000")
    npc:ModifyNPCStat("min_hit", "240")
    npc:ModifyNPCStat("max_hit", stage == raid and "1950" or "1350")
    if stage == survey and type_id % 3 == 0 then npc:ModifyNPCStat("see_invis", "1") end
    npc:SetHP(npc:GetMaxHP())
end

function M.loot(e)
    local stage, client = stage_here(), e.other
    local item = e.item and e.item.valid and e.item:GetID()
    if stage == survey then
        if item == layout.items.figurine then
            return member(client, stage) and client:IsTaskActivityActive(stage.id, 1) and 0 or 1
        end
        if item == layout.items.stone then
            return member(client, stage) and client:IsTaskActivityActive(stage.id, 3) and 0 or 1
        end
    end
    if stage == question and item == layout.items.scroll then
        return member(client, stage) and client:IsTaskActivityActive(stage.id, 2) and 0 or 1
    end
    return 0
end

function M.tick(e)
    if e.timer ~= "tunare_shrine" then return end
    local stage, client = stage_here(), e.self
    if not member(client, stage) then return end
    recover_unfinished(stage)
    if stage == survey and nearby(client, 56, 111, 222.875, 45, 18) then
        por.advance(client, survey.id, 0)
    end
    if stage == survey and not survey_ready(client) then return end

    for npc in eq.get_entity_list():GetNPCList().entries do
        local id = npc:GetNPCTypeID()
        if ((id >= 379000 and id <= 379013) or id == 379951) and not npc:IsEngaged()
            and nearby(client, npc:GetX(), npc:GetY(), npc:GetZ(), 32, 18)
            and not client:IsInvisible(npc) and npc:CheckLoS(client) then
            npc:AddToHateList(client, 1)
        end
    end
end

return M
