local layout = require("rof2_tss_ashengate_layout")
local route = require("rof2_por_progression")
local read_one = require("rof2_dodh_scene_receipts").read_one

local M = {}
local warning = "tss_ashengate:jenray_weapons"

local function finished_raid(client, task, final_activity)
    return not not read_one([[
        SELECT 1
        FROM completed_shared_tasks t
        JOIN completed_shared_task_members m ON m.shared_task_id = t.id
        JOIN completed_shared_task_activity_state a ON a.shared_task_id = t.id
        WHERE m.character_id = ? AND t.task_id = ? AND t.completion_time IS NOT NULL
          AND a.activity_id = ? AND a.done_count = 1 AND a.completed_time IS NOT NULL
        LIMIT 1
    ]], {client:CharacterID(), task, final_activity})
end

local function finished_kellak_route(client)
    for task = 3040702, 3040705 do
        if not client:IsTaskCompleted(task) then return false end
    end
    return finished_raid(client, 920360, 1)
end

local function qualified_request(client, spec)
    return client:GetLevel() >= 75 and client:IsTaskCompleted(spec.vergalid)
        and client:HasAshengateFaction(spec.version, false)
        and finished_kellak_route(client)
        and (finished_raid(client, 920361, 4) or client:HasAshengateVergalidMark())
end

local function cycle_position(client, spec)
    for _, task in ipairs(spec.cycle) do
        if client:IsTaskActive(task) then return end
    end

    local row = read_one([[
        SELECT COALESCE(MAX(CASE WHEN taskid = ? THEN completedtime END), 0),
               COALESCE(MAX(CASE WHEN taskid = ? THEN completedtime END), 0),
               COALESCE(MAX(CASE WHEN taskid = ? THEN completedtime END), 0)
        FROM completed_tasks
        WHERE charid = ? AND activityid = -1 AND taskid IN (?, ?, ?)
    ]], {spec.cycle[1], spec.cycle[2], spec.cycle[3], client:CharacterID(),
        spec.cycle[1], spec.cycle[2], spec.cycle[3]})
    if not row then return end

    local first, second, third = tonumber(row[1]), tonumber(row[2]), tonumber(row[3])
    if not first or not second or not third then return end
    if first <= third then return 1 end
    if second < first then return 2 end
    return 3
end

local function offer_cycle(client, npc, spec, limit)
    local stage = cycle_position(client, spec)
    if not stage or (limit and stage ~= limit) or client:GetLevel() < spec.level[stage] then
        return false
    end
    client:TaskSelectorFromNPC(npc, {spec.cycle[stage]})
    return true
end

local function offer_side(client, npc, spec)
    if client:GetLevel() < 75 or not finished_kellak_route(client)
        or not client:HasAshengateFaction(spec.version, true) then return false end
    if not client:IsTaskCompleted(spec.vergalid) then
        if client:IsTaskActive(spec.vergalid) then return false end
        client:TaskSelectorFromNPC(npc, {spec.vergalid})
        return true
    end
    if not client:HasAshengateFaction(spec.version, false)
        or not finished_raid(client, 920361, 4)
        or client:IsTaskCompleted(spec.wing_task)
        or client:IsTaskActive(spec.wing_task) then return false end
    client:TaskSelectorFromNPC(npc, {spec.wing_task})
    return true
end

local function native_hail(npc, words)
    return words == "hail" or words == "hail, " .. npc:GetCleanName():lower()
end

local function jenray_will_speak(npc, client, hail)
    if client:GetItemIDAt(13) <= 0 and client:GetItemIDAt(14) <= 0 then
        client:DeleteEntityVariable(warning)
        return true
    end
    local warned = tonumber(client:GetEntityVariable(warning)) or 0
    if hail and warned > os.time() then
        npc:AddToHateList(client, 1)
        client:DeleteEntityVariable(warning)
    else
        npc:Say("Speak to me with your weapons sheathed.")
        if hail then client:SetEntityVariable(warning, tostring(os.time() + 60)) end
    end
    return false
end

function M.accepted(e, key)
    local spec = layout[key]
    local client = e.other
    if e.task_id ~= spec.cycle[1] or not client.valid or not client:IsTaskActive(e.task_id)
        or not route.public(e.self, 405, spec.npc, spec.spawn) then return end
    -- The tools are issued on the actual personal task acceptance.
    if client:GetInventory():HasItem(spec.supply, 1, 39) < 0 then
        client:SummonItem(spec.supply)
    end
end

function M.say(e, key)
    local spec = layout[key]
    local npc, client = e.self, e.other
    if not route.public(npc, 405, spec.npc, spec.spawn) or not client.valid then return end
    local words = route.words(e)
    local hail = native_hail(npc, words)
        or (key == "west" and words == "hail, jenray, envoy of ro")
    local entry = words == "enter ashengate" or words == "i am ready to enter ashengate"
    if entry then
        local dz = client:GetExpedition()
        if dz.valid and dz:GetZoneID() == spec.zone and dz:GetZoneVersion() == spec.version
            and dz:GetName() == spec.name then
            client:MovePCDynamicZone(spec.zone, spec.version, false)
        else
            npc:Say("You are not assigned to this Ashengate expedition.")
        end
        return
    end

    local request = key == "east" and words == "i am ready to challenge him"
        or key == "west" and words == "give me a greater challenge"
    local general = words == "essential tasks" or words == "what information" or words == "information"
    local gifts = words == "gifts" or words == "what gifts" or words == "superiority"
    local greater = words == "greater things" or words == "yes, i have greater things on my mind"
        or words == "greater challenge"
    if key == "west" and (hail or request or gifts or greater)
        and not jenray_will_speak(npc, client, hail) then return end
    if request then
        local raid = client:GetRaid()
        if not raid.valid or not raid:IsLeader(client) then
            npc:Say("Your raid leader must make this request.")
        elseif not qualified_request(client, spec) then
            npc:Say("You have yet to earn the right to lead this expedition.")
        elseif client:GetExpedition().valid then
            npc:Say("You are already assigned to an expedition.")
        else
            client:CreateExpeditionFromTemplate(spec.template)
        end
        return
    end

    if hail then
        if key == "east" then
            if route.advance(client, spec.vergalid, 3) or route.advance(client, spec.cycle[3], 2) then return end
            npc:Say("Tell me what you " .. eq.say_link("despise") .. ", or ask about my "
                .. eq.say_link("essential tasks") .. ".")
            if qualified_request(client, spec) then
                npc:Say("Say " .. eq.say_link("I am ready to challenge him") .. " when your raid is assembled.")
            end
        else
            client:ReconcileAshengateGive()
            if route.advance(client, spec.cycle[2], 1) or route.advance(client, spec.wing_task, 1) then return end
            npc:Say("Hear of the " .. eq.say_link("gifts") .. " of Solusek Ro, or speak of "
                .. eq.say_link("greater things") .. ".")
        end
        return
    end

    if key == "east" then
        if words == "despise" then
            npc:Say("Tell me if " .. eq.say_link("I despise the ground") .. " or "
                .. eq.say_link("I despise him") .. " is closer to the truth.")
        elseif words == "i despise the ground" then
            if not offer_cycle(client, npc, spec, 1) then
                npc:Say("Ask about my " .. eq.say_link("essential tasks") .. ".")
            end
        elseif words == "i despise him" then
            offer_cycle(client, npc, spec, 2)
        elseif words == "essential tasks" then
            if not offer_cycle(client, npc, spec, 3) then
                npc:Say("I have " .. eq.say_link("information") .. " for those who have earned my trust.")
            end
        elseif general then
            if not offer_side(client, npc, spec) then
                npc:Say("I have no further information for you yet.")
            end
        end
    elseif words == "superiority" then
        if not offer_side(client, npc, spec) then offer_cycle(client, npc, spec) end
    elseif gifts then
        npc:Say("Ask about the " .. eq.say_link("superiority") .. " of Solusek Ro.")
    elseif greater and qualified_request(client, spec) then
        npc:Say("Say " .. eq.say_link("Give me a greater challenge") .. " when your raid is assembled.")
    end
end

return M
