local por = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local read_one = require("rof2_dodh_scene_receipts").read_one

local M = {}
local version = 20
local deathknell_zone = 381
local lockout_seconds = 5 * 86400
local raids = {
    daosheen = {zone=371, version=version, name="Daosheen the Firstborn", template=920340},
    sullon = {zone=375, version=version, name="Sullon Zek", template=920341},
    suchun = {zone=377, version=version, name="Suchun the Blood Warden", template=920342},
    deathknell = {zone=deathknell_zone, version=version, name="Deathknell", template=920343},
}
M.raids = raids

local function expedition(client, binding)
    local dz = client:GetExpedition()
    if dz.valid and dz:GetName() == binding.name and dz:GetZoneID() == binding.zone
        and dz:GetZoneVersion() == version and dz:GetInstanceID() > 0 then
        return dz
    end
end

local function theater_admission(client, dz)
    if client:HasZoneFlag(deathknell_zone) or client:GetLevel() >= 72 then return true end
    local raid = client:GetRaid()
    if not raid.valid or dz:GetSecondsRemaining() <= 0 then return false end
    -- Flag sharing applies only to the overlapping live raid and expedition roster.
    local counts = read_one([[
        SELECT COUNT(DISTINCT r.charid),
               COUNT(DISTINCT f.charID),
               COUNT(DISTINCT CASE WHEN r.charid=? THEN r.charid END)
        FROM raid_members r
        JOIN dynamic_zone_members m ON m.character_id=r.charid
        JOIN dynamic_zones d ON d.id=m.dynamic_zone_id
        JOIN instance_list i ON i.id=d.instance_id
        LEFT JOIN zone_flags f ON f.charID=r.charid AND f.zoneID=381
        WHERE r.raidid=? AND r.charid>0 AND d.id=? AND d.uuid=? AND i.id=?
          AND i.zone=381 AND i.version=20
          AND (i.never_expires=1 OR i.expire_at>UNIX_TIMESTAMP())
    ]], {client:CharacterID(), raid:GetID(), dz:GetID(), dz:GetUUID(), dz:GetInstanceID()})
    if not counts or tonumber(counts[3]) ~= 1 then return false end
    local roster, flagged = tonumber(counts[1]), tonumber(counts[2])
    return roster and flagged and roster >= 6 and 100 * flagged >= 85 * roster
end

function M.move(client, key)
    local binding = raids[key]
    local dz = expedition(client, binding)
    if not dz then return false end
    if key == "deathknell" and not theater_admission(client, dz) then
        client:Message(MT.Yellow, "You have not earned passage into Deathknell.")
        return true
    end
    client:MovePCDynamicZone(binding.zone, version, true)
    return true
end

local function completed_shared(client, task)
    local row = read_one([[
        SELECT 1
        FROM completed_shared_tasks t
        JOIN completed_shared_task_members m ON m.shared_task_id=t.id
        WHERE m.character_id=? AND t.task_id=? AND t.completion_time IS NOT NULL
        LIMIT 1
    ]], {client:CharacterID(), task})
    return row and tonumber(row[1]) == 1
end
M.completed_shared = completed_shared

local qualifies = {
    daosheen = function(c) return c:IsTaskCompleted(920344) end,
    sullon = function(c)
        return c:IsTaskCompleted(920345) and c:CountItem(88099) > 0 and completed_shared(c, 920305)
    end,
    suchun = function(c) return completed_shared(c, 920309) end,
    deathknell = function(c) return c:HasZoneFlag(deathknell_zone) end,
}

function M.request(client, key)
    if client:GetExpedition().valid then
        client:Message(MT.Yellow, "You are already part of an expedition.")
        return
    end
    local raid = client:GetRaid()
    if not raid.valid or not raid:IsLeader(client) then
        client:Message(MT.Yellow, "Your raid leader must make the request.")
        return
    end
    if not qualifies[key](client) then
        client:Message(MT.Yellow, "You have not earned the right to request this expedition.")
        return
    end
    client:CreateExpeditionFromTemplate(raids[key].template)
end

function M.arrival(client, key, origin)
    local dz = runtime.scope(client, raids[key])
    if not dz or (key == "deathknell" and not theater_admission(client, dz)) then
        client:MovePC(origin[1], origin[2], origin[3], origin[4], origin[5])
        return false
    end
    return true
end

local function lockout(key)
    return raids[key].name .. ": victory"
end

M.lockout = lockout

function M.won(dz, key)
    return dz:HasLockout(lockout(key))
end

function M.credit(client, key, dz)
    local sql = [[SELECT 1 FROM por_deathknell_raid_credit
        WHERE character_id=? AND stage=?]]
    local values = {client:CharacterID(), key}
    if dz then
        sql = sql .. " AND dz_uuid=? AND instance_id=?"
        values[3], values[4] = dz:GetUUID(), dz:GetInstanceID()
    end
    local row = read_one(sql .. " LIMIT 1", values)
    return row and tonumber(row[1]) == 1
end

function M.capture(dz, key)
    local binding = raids[key]
    if not binding or key == "deathknell" or not runtime.in_instance(dz) or M.won(dz, key) then
        return
    end
    local members = {}
    for client in eq.get_entity_list():GetClientList().entries do
        local owned = runtime.scope(client, binding)
        if owned and owned:GetUUID() == dz:GetUUID() then
            members[#members + 1] = client:CharacterID()
        end
    end
    if #members == 0 or #members > 54 then return end
    return {members=members, key=key, dz=dz}
end

-- Persist the physical roster frozen at the native terminal death.
function M.commit(pending)
    if not pending or not runtime.in_instance(pending.dz) then return end
    local dz, key, members = pending.dz, pending.key, pending.members
    if M.won(dz, key) then return end
    local binding = raids[key]
    local placeholders = {}
    local args = {key, dz:GetUUID(), dz:GetInstanceID(), binding.name, binding.zone, version}
    local values = {key, dz:GetUUID(), dz:GetInstanceID()}
    for _, id in ipairs(members) do
        placeholders[#placeholders + 1] = "?"
        args[#args + 1] = id
        values[#values + 1] = id
    end
    local db, stmt
    local ok = pcall(function()
        db = Database(Database.Default)
        stmt = db:prepare([[
            INSERT INTO por_deathknell_raid_credit (dz_uuid,stage,character_id,instance_id)
            SELECT z.uuid,?,m.character_id,z.instance_id
            FROM dynamic_zones z
            JOIN dynamic_zone_members m ON m.dynamic_zone_id=z.id
            JOIN instance_list i ON i.id=z.instance_id
            WHERE z.uuid=? AND z.instance_id=? AND z.name=? AND i.zone=? AND i.version=?
              AND m.character_id IN (]] .. table.concat(placeholders, ",") .. [[)
            ON DUPLICATE KEY UPDATE character_id=VALUES(character_id)
        ]])
        stmt:execute(args)
    end)
    if stmt then stmt:close() end
    if db then db:close() end
    if not ok then return end
    local row = read_one([[
        SELECT COUNT(*) FROM por_deathknell_raid_credit
        WHERE stage=? AND dz_uuid=? AND instance_id=?
          AND character_id IN (]] .. table.concat(placeholders, ",") .. [[)
    ]], values)
    if not row or tonumber(row[1]) ~= #members then return end
    dz:AddLockout(lockout(key), lockout_seconds)
    return members
end

function M.connected(id, dz, key)
    local client = eq.get_entity_list():GetClientByCharID(id)
    local owned = runtime.scope(client, raids[key])
    if owned and owned:GetUUID() == dz:GetUUID() then return client end
end

function M.terminal(e, key)
    local client = por.player(e.other)
    if client then return runtime.scope(client, raids[key]) end
end

return M
