local unpack_args = table.unpack or unpack
local layout = require("rof2_por_tunare_layout")
local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local read_one = require("rof2_dodh_scene_receipts").read_one

local M = {}
local mark = "por_tunare:reward_run"
local picks = {
    [920346] = {npc = 379947, context = "54554E3101"},
    [920347] = {npc = 379948, context = "54554E3102"},
    [920348] = {npc = 379949},
}
local loot = {
    [39121] = true, [39124] = true, [39127] = true, [39122] = true, [39125] = true,
    [39128] = true, [39135] = true, [39138] = true, [39141] = true,
}

local function write(sql, params)
    local db, stmt
    local ok = pcall(function()
        db = Database(Database.Default)
        stmt = db:prepare(sql)
        stmt:execute(params)
    end)
    if stmt then stmt:close() end
    if db then db:close() end
    return ok
end

function M.place(task, run, character, point)
    local row = read_one("SELECT 1 FROM por_tunare_reward_chests WHERE shared_task_id=?", {run})
    if row ~= false then return end

    local chest = eq.spawn2(picks[task].npc, 0, 0, unpack_args(point))
    if not chest.valid then return end

    if not write([[
        INSERT INTO por_tunare_reward_chests (shared_task_id,character_id,task_id,created_at)
        VALUES (?,?,?,UNIX_TIMESTAMP())
    ]], {run, character, task}) or not read_one([[
        SELECT 1 FROM por_tunare_reward_chests WHERE shared_task_id=? AND character_id=? AND task_id=?
    ]], {run, character, task}) then
        chest:Depop(false)
        return
    end
    chest:SetEntityVariable(mark, tostring(run))
end

function M.shalowen(client)
    for task, selection in pairs(picks) do
        if selection.context then
            local row = read_one([[
                SELECT j.shared_task_id
                FROM kithicor_poison_custody j
                JOIN completed_shared_tasks st ON st.id=j.shared_task_id AND st.task_id=?
                JOIN completed_shared_task_members m ON m.shared_task_id=st.id AND m.character_id=j.character_id
                LEFT JOIN por_tunare_reward_chests c ON c.shared_task_id=st.id
                WHERE j.recipe_kind=4 AND j.recipe_context=UNHEX(?) AND j.character_id=?
                   AND j.instance_id=0 AND j.npc_type_id=378059 AND j.npc_spawn_id=104960
                   AND j.physical=1 AND j.settled=1 AND j.abort_only=0
                   AND st.completion_time IS NOT NULL AND c.shared_task_id IS NULL
                ORDER BY st.id LIMIT 1
            ]], {task, selection.context, client:CharacterID()})
            if row then M.place(task, tostring(row[1]), client:CharacterID(), {9,126,88.125,504}) end
        end
    end
end

function M.death(e)
    if not por.current(layout.forest, 0, 0) or not e.killed or not e.killed.valid then return end
    local npc = e.killed
    if npc:GetNPCTypeID() < 379947 or npc:GetNPCTypeID() > 379949 then return end
    local corpse = runtime.corpse(e)
    local run = npc:GetEntityVariable(mark)
    if corpse and run ~= "" then
        corpse:SetEntityVariable(mark, run)
    end
end

function M.loot(e)
    if not por.current(layout.forest, 0, 0) or not e.item or not e.item.valid or not loot[e.item:GetID()]
        or not e.corpse or not e.corpse.valid then return 0 end
    local run = e.corpse:GetEntityVariable(mark)
    if run == "" then
        local id = e.corpse:GetNPCTypeID()
        if id >= 379947 and id <= 379949 then return 1 end
        return 0
    end

    local active_key = e.other:IsTaskActive(layout.stages[3].id) and e.other:GetSharedTaskIdentity() == run
    local row = read_one([[
        SELECT c.task_id
        FROM por_tunare_reward_chests c
        JOIN completed_shared_task_members m ON m.shared_task_id=c.shared_task_id
        WHERE c.shared_task_id=? AND m.character_id=?
    ]], {run, e.other:CharacterID()})
    if row or active_key then return 0 end
    return 1
end

return M
