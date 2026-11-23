local unpack_args = table.unpack or unpack
local layout = require("rof2_por_tunare_layout")
local por = require("rof2_por_progression")
local read_one = require("rof2_dodh_scene_receipts").read_one
local rewards = require("rof2_por_tunare_rewards")

local M = {}
local chest_watch = "por_tunare:chest_watch"
local prompts = {
    "interested in observing",
    "priest nalderin",
    "crafter",
    "gather a force",
}

local function completed(client, task)
    if task <= 0 then return false end
    return not not read_one([[
        SELECT 1
        FROM completed_shared_tasks t
        JOIN completed_shared_task_members m ON m.shared_task_id=t.id
        WHERE m.character_id=? AND t.task_id=? AND t.completion_time IS NOT NULL
        LIMIT 1
    ]], {client:CharacterID(), task})
end

local function eligible(client, index)
    local stage = layout.stages[index]
    if client:GetLevel() < stage.level then return false end
    if index == 2 then return completed(client, layout.stages[1].id) end
    if index == 3 then
        return completed(client, layout.stages[1].id) and completed(client, layout.stages[2].id)
    end
    return true
end

function M.shalowen(e)
    if not e.other.valid or not por.public(e.self, layout.forest, layout.shalowen.npc, layout.shalowen.spawn) then
        return
    end

    local client, words = e.other, por.words(e)
    if words == "i would interested in observing them" or words == "i would be interested in observing them"
        or words == "i am interested in observing them" then words = prompts[1] end
    if words == "what crafter" then words = prompts[3] end
    if words == "hail" or words == "corrupted souls" then
        client:ReconcileTunareGive()
        rewards.shalowen(client)
        local choices = {}
        for index, keywords in ipairs(prompts) do
            if eligible(client, index) then choices[#choices + 1] = eq.say_link(keywords) end
        end
        if #choices > 0 then
            e.self:Say("I am investigating the priests who withdrew into the shrine. Ask about "
                .. table.concat(choices, " or ") .. ".")
        end
        return
    end

    for index, keywords in ipairs(prompts) do
        if (words == keywords or words == "i can " .. keywords) and eligible(client, index) then
            if layout.stages[index].raid then
                local raid = client:GetRaid()
                if not raid.valid or not raid:IsLeader(client) then
                    e.self:Say("Have your raid leader ask me about the corruption.")
                    return
                end
            end
            client:TaskSelectorFromNPC(e.self, {layout.stages[index].id})
            return
        end
    end
end

function M.follow_up(e)
    if e.timer ~= "tunare_woods" or not por.current(layout.forest, 0, 0) then return end
    local client = e.self
    if not client.valid then return end
    local dx, dy = client:GetX() - 5, client:GetY() - 127
    if dx*dx + dy*dy > 55*55 or math.abs(client:GetZ() - 88.125) > 20 then return end

    local now = os.time()
    if client:IsTaskActivityActive(layout.stages[1].id, 5) or
        client:IsTaskActivityActive(layout.stages[2].id, 3) then
        client:SetEntityVariable(chest_watch, tostring(now + 90))
        return
    end
    local deadline = tonumber(client:GetEntityVariable(chest_watch))
    if deadline and now <= deadline and now % 3 == 0 then
        client:ReconcileTunareGive()
        rewards.shalowen(client)
    end
end

function M.retained_raid_instance(client)
    if not client.valid then return end
    local row = read_one([[
        SELECT i.id
        FROM completed_shared_tasks t
        JOIN completed_shared_task_members m ON m.shared_task_id=t.id
        JOIN shared_task_dynamic_zones sd ON sd.shared_task_id=t.id
        JOIN dynamic_zones d ON d.id=sd.dynamic_zone_id
        JOIN dynamic_zone_members dm ON dm.dynamic_zone_id=d.id AND dm.character_id=m.character_id
        JOIN instance_list i ON i.id=d.instance_id
        WHERE m.character_id=? AND t.task_id=? AND t.completion_time IS NOT NULL
          AND d.type=4 AND i.zone=? AND i.version=? AND i.expire_at>UNIX_TIMESTAMP()
        LIMIT 2
    ]], {client:CharacterID(), layout.stages[4].id, layout.shrine, layout.stages[4].version})
    local instance = row and tonumber(row[1])
    if instance and instance > 0 then return instance end
end

function M.enter(e)
    if not por.current(layout.forest, 0, 0) or not e.self.valid or not e.door.valid then return 0 end
    local door = layout.entrance
    if e.door:GetDoorDBID() ~= door.db or e.door:GetDoorID() ~= door.door then return 0 end

    local x, y, z = e.self:GetX() - door.x, e.self:GetY() - door.y, e.self:GetZ() - door.z
    if x*x + y*y > 30*30 or math.abs(z) > 14 then return 1 end

    for _, stage in ipairs(layout.stages) do
        if stage.version and por.enter(e.self, stage.id,
            layout.shrine, stage.version, unpack_args(layout.shrine_landing)) then
            return 1
        end
    end
    local retained = M.retained_raid_instance(e.self)
    if retained then
        e.self:MovePCInstance(layout.shrine, retained, unpack_args(layout.shrine_landing))
        return 1
    end
    e.self:Message(MT.Yellow, "You have no current mission in Tunare's Shrine.")
    return 1
end

return M
