local por = require("rof2_por_progression")
local M = {}

-- Every state read and write joins the caller to the still-owned private
-- instance and the latest active shared occurrence. Identical map versions
-- can never borrow each other's local sequence.
local joins = [[
JOIN shared_task_members m ON m.shared_task_id=st.id AND m.character_id=?
JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=st.id
JOIN dynamic_zones dz ON dz.id=sd.dynamic_zone_id AND dz.type=4
JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=dz.id AND dm.character_id=m.character_id
JOIN instance_list i ON i.id=dz.instance_id AND i.zone=? AND i.version=?
]]

local active = [[
WHERE st.id=CAST(? AS UNSIGNED) AND st.task_id=? AND dz.instance_id=?
AND st.completion_time IS NULL AND (st.expire_time IS NULL OR st.expire_time>NOW())
AND i.never_expires=0 AND i.expire_at>UNIX_TIMESTAMP()
]]

local function request(client,task,zone,version)
    if not por.inside(client,task,zone,version) then return end
    return {client:CharacterID(),zone,version,client:GetSharedTaskIdentity(),task,eq.get_zone():GetInstanceID()}
end

local function run(sql, params, write)
    local db, stmt
    local ok, value, owner = pcall(function()
        db=Database(Database.Default)
        stmt=db:prepare(sql)
        stmt:execute(params)
        if write then return tonumber(stmt:rows_affected()) end
        local row=stmt:fetch_array()
        if not row then return false end
        return tonumber(row[1]),tonumber(row[2])
    end)
    if stmt then stmt:close() end
    if db then db:close() end
    if ok then return value,owner end
end

function M.read(client,task,zone,version,anchor)
    local args=request(client,task,zone,version)
    if not args then return end
    args[#args+1]=anchor
    return run("SELECT state.scene_step,state.character_id FROM shared_tasks st " .. joins ..
        "JOIN por_group_scene_state state ON state.shared_task_id=st.id AND state.task_id=st.task_id " ..
        "AND state.instance_id=dz.instance_id " .. active .. " AND state.anchor_id=?",args,false)
end

function M.start(client,task,zone,version,anchor,step)
    local args=request(client,task,zone,version)
    if not args then return false end
    table.insert(args,1,step)
    table.insert(args,1,anchor)
    local result=run([[
INSERT IGNORE INTO por_group_scene_state
(shared_task_id,task_id,instance_id,anchor_id,character_id,scene_step)
SELECT st.id,st.task_id,dz.instance_id,?,m.character_id,?
 FROM shared_tasks st
]] .. joins .. active,args,true)
    return result==1
end

function M.advance(client,task,zone,version,from,to)
    local args=request(client,task,zone,version)
    if not args then return false end
    args[#args+1]=from
    -- In an UPDATE JOIN query the owner join parameters precede the SET value.
    table.insert(args,4,to)
    local result=run("UPDATE shared_tasks st " ..
        joins ..
        "JOIN por_group_scene_state state ON state.shared_task_id=st.id AND state.task_id=st.task_id " ..
        "AND state.instance_id=dz.instance_id AND state.anchor_id=0 SET state.scene_step=? " ..
        active .. " AND state.scene_step=?",args,true)
    return result==1
end

function M.egg_debited(client)
    local args=request(client,920302,371,12)
    if not args then return false end
    return run("SELECT COUNT(inv.slot_id),0 FROM shared_tasks st " .. joins ..
        "JOIN por_group_scene_state state ON state.shared_task_id=st.id AND state.task_id=st.task_id " ..
        "AND state.instance_id=dz.instance_id AND state.anchor_id=0 AND state.character_id=m.character_id " ..
        "LEFT JOIN inventory inv ON inv.character_id=state.character_id AND inv.item_id=36145 " ..
        active .. " AND state.scene_step=0 GROUP BY state.shared_task_id",args,false)==0
end

return M
