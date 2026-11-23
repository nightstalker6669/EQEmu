local layout = require("rof2_por_tunare_layout")
local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local read_one = require("rof2_dodh_scene_receipts").read_one
local rewards = require("rof2_por_tunare_rewards")

local M = {}
local task = layout.stages[3].id
local orgaf = layout.orgaf
local key = layout.items.key
local mark = "por_tunare:orgaf"
local sight = {}
local fights = {}

for index, actor in ipairs(layout.treants) do
    sight[actor.spawn] = {actor = actor, activity = 2 * (index - 1)}
end

local function in_forest()
    return por.current(layout.forest, 0, 0)
end

local function shares(client, run)
    return client and client.valid and client:IsTaskActive(task)
        and client:GetSharedTaskIdentity() == tostring(run)
end

local function nearby(client, npc, range)
    local x, y = client:GetX() - npc:GetX(), client:GetY() - npc:GetY()
    return x*x + y*y <= range*range and math.abs(client:GetZ() - npc:GetZ()) <= 25
end

local function lease()
    return read_one([[
        SELECT shared_task_id,character_id,fight_at,state
        FROM por_tunare_orgaf WHERE spawn_id=?
    ]], {orgaf.spawn})
end

local function mark_dead(run)
    local db, statement
    local ok = pcall(function()
        db = Database(Database.Default)
        statement = db:prepare([[
            UPDATE por_tunare_orgaf SET state=1,dead_at=UNIX_TIMESTAMP()
            WHERE spawn_id=? AND shared_task_id=? AND state=0
        ]])
        statement:execute({orgaf.spawn, tostring(run)})
    end)
    if statement then statement:close() end
    if db then db:close() end
    if not ok then return false end
    local row = lease()
    return row and tostring(row[1]) == tostring(run) and tonumber(row[4]) == 1
end

function M.player_enter(e)
    if in_forest() then eq.set_timer("tunare_woods", 1000) end
end

function M.player_tick(e)
    local client = e.self
    if e.timer ~= "tunare_woods" or not in_forest() or not client:IsTaskActive(task) then return end
    for npc in eq.get_entity_list():GetNPCList().entries do
        local objective = sight[npc:GetSpawnPointID()]
        if objective and npc:GetNPCTypeID() == objective.actor.npc
            and client:IsTaskActivityActive(task, objective.activity) and nearby(client, npc, 75) then
            por.advance(client, task, objective.activity)
        end
    end
end

function M.treant_damage(e)
    local npc, client = e.self, por.player(e.other)
    local objective = sight[npc:GetSpawnPointID()]
    if not objective or not por.public(npc, layout.forest, objective.actor.npc, objective.actor.spawn)
        or not client or not client:IsTaskActivityActive(task, objective.activity + 1) then return 0 end
    local id = npc:GetID()
    if not fights[id] then
        npc:ModifyNPCStat("max_hp", "135000")
        npc:ModifyNPCStat("min_hit", "450")
        npc:ModifyNPCStat("max_hit", "2200")
        npc:SetHP(npc:GetMaxHP())
        fights[id] = {wave = 0, cast_at = os.time() + 12, adds = {}}
        eq.set_timer("tunare_treant", 1000)
    end
    return 0
end

function M.treant_tick(e)
    if e.timer ~= "tunare_treant" then return end
    local npc = e.self
    local fight = fights[npc:GetID()]
    if not fight then return end
    local target = por.player(npc:GetHateTop())
    if not target then
        for _, id in ipairs(fight.adds) do
            local add = eq.get_entity_list():GetNPCByID(id)
            if add.valid then add:Depop(false) end
        end
        fights[npc:GetID()] = nil
        eq.stop_timer(e.timer)
        return
    end
    if os.time() >= fight.cast_at then
        npc:CastSpell(8599, target:GetID())
        fight.cast_at = os.time() + 30
    end
    if npc:GetNPCTypeID() ~= 378147 or fight.wave == 4 then return end
    if npc:GetHPRatio() <= 80 - 20 * fight.wave then
        fight.wave = fight.wave + 1
        for _, side in ipairs({-1,1}) do
            local add = eq.spawn2(379950, 0, 0, npc:GetX() + side*8, npc:GetY() + 5, npc:GetZ(), 0)
            if add.valid then
                fight.adds[#fight.adds+1] = add:GetID()
                add:AddToHateList(target, 1)
            end
        end
    end
end

function M.orgaf_say(e)
    if not por.public(e.self, layout.forest, orgaf.npc, orgaf.spawn)
        or not e.other.valid or not nearby(e.other, e.self, 45) or e.self:IsEngaged() then return end
    local words = por.words(e)
    if words == "hail" then
        e.other:ReconcileTunareGive()
        por.advance(e.other, task, 8)
        e.self:Say("What you want with Orgaf? " .. eq.say_link("Shalowen sent me"))
    elseif words == "shalowen sent me" then
        e.self:Say("Orgaf can make a special key for friends of Shalowen. Bring all four pieces of magical wood.")
    end
end

function M.orgaf_spawn(e)
    if por.public(e.self, layout.forest, orgaf.npc, orgaf.spawn) then
        eq.set_timer("tunare_orgaf", 3000)
    end
end

function M.orgaf_tick(e)
    if e.timer ~= "tunare_orgaf" or not por.public(e.self, layout.forest, orgaf.npc, orgaf.spawn) then return end
    local npc = e.self
    if npc:IsEngaged() then return end
    local row = lease()
    if not row or tonumber(row[4]) ~= 0 then return end
    if npc:GetEntityVariable(mark) == tostring(row[1]) and read_one([[
        SELECT 1 FROM shared_tasks
        WHERE id=? AND task_id=? AND completion_time IS NULL
          AND (expire_time IS NULL OR expire_time>NOW())
    ]], {tostring(row[1]), task}) == false then
        npc:DeleteEntityVariable(mark)
        npc:DeleteEntityVariable("por_tunare:orgaf_hostile")
        npc:ModifyNPCStat("max_hp", "5904")
        npc:ModifyNPCStat("min_hit", "0")
        npc:ModifyNPCStat("max_hit", "4")
        npc:SetHP(npc:GetMaxHP())
        return
    end

    for client in eq.get_entity_list():GetClientList().entries do
        if shares(client, row[1]) and nearby(client, npc, 65) then
            local prior = npc:GetEntityVariable(mark)
            if prior ~= tostring(row[1]) then
                npc:SetEntityVariable(mark, tostring(row[1]))
                npc:Say("You got the magical wood for Orgaf. Wait here and Orgaf make key.")
            end
            -- Requiring the stock next activity also waits for physical Give recovery.
            if os.time() >= tonumber(row[3]) and client:IsTaskActivityActive(task, 10) then
                npc:Say("Orgaf done with key. Orcs need it more. The Tribe of the Nodgha shall crush you!")
                if npc:GetEntityVariable("por_tunare:orgaf_hostile") ~= tostring(row[1]) then
                    npc:ModifyNPCStat("max_hp", "245000")
                    npc:ModifyNPCStat("min_hit", "320")
                    npc:ModifyNPCStat("max_hit", "2200")
                    npc:SetHP(npc:GetMaxHP())
                    npc:SetEntityVariable("por_tunare:orgaf_hostile", tostring(row[1]))
                end
                npc:AddToHateList(client, 1)
            end
            return
        end
    end
end

function M.orgaf_damage(e)
    if not por.public(e.self, layout.forest, orgaf.npc, orgaf.spawn) then return 0 end
    local run = e.self:GetEntityVariable("por_tunare:orgaf_hostile")
    if run == "" then return 0 end
    if not shares(por.player(e.other), run) then return -1 end
    return 0
end

function M.death(e)
    if not in_forest() or not e.killed or not e.killed.valid then return end
    local npc, client = e.killed, por.player(e.other)
    if sight[npc:GetSpawnPointID()] then fights[e.killed_entity_id] = nil end
    local run = npc:GetEntityVariable("por_tunare:orgaf_hostile")
    if npc:GetNPCTypeID() ~= orgaf.npc or npc:GetSpawnPointID() ~= orgaf.spawn
        or run == "" or not shares(client, run) or not client:IsTaskActivityActive(task, 10) then return end
    local row, corpse = lease(), runtime.corpse(e)
    if not row or tostring(row[1]) ~= run or tonumber(row[4]) ~= 0 or not corpse then return end

    corpse:AddItem(key, 1)
    if corpse:CountItem(key) ~= 1 then return end
    corpse:SetEntityVariable(mark, run)
    corpse:ResetDecayTimer()
    if not mark_dead(run) then return end
    rewards.place(task, run, client:CharacterID(), {corpse:GetX() + 4, corpse:GetY() + 3, corpse:GetZ(), 0})
end

function M.loot(e)
    local item = e.item and e.item.valid and e.item:GetID()
    if not in_forest() or item ~= key then return 0 end
    if not e.corpse or not e.corpse.valid or not e.corpse:IsNPCCorpse()
        or e.corpse:GetNPCTypeID() ~= orgaf.npc then return 1 end
    local run = e.corpse:GetEntityVariable(mark)
    local row = lease()
    if run == "" or not row or tostring(row[1]) ~= run or tonumber(row[4]) ~= 1
        or not shares(e.other, run) or not e.other:IsTaskActivityActive(task, 10) then return 1 end
    return 0
end

return M
