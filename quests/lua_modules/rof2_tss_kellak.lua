local route = require("rof2_por_progression")
local runtime = require("rof2_dodh_raid_runtime")
local raid = require("rof2_tss_vergalid_raid")
local layout = require("rof2_tss_vergalid_layout")

local M = {}
local binding = layout.kellak
local timer = "tss_kellak_combat"
local state

local function actor(npc)
    return npc and npc.valid and npc:GetNPCTypeID() == layout.actors.kellak
        and npc:GetSpawnPointID() == binding.placement
end

function M.spawn(e)
    if not actor(e.self) or not route.current(404, 20) or eq.get_zone():GetInstanceID() == 0 then return end
    e.self:SetPseudoRoot(true)
end

function M.damage(e)
    if not actor(e.self) then return -1 end
    local client = route.player(e.other)
    local uuid = raid.active(client, binding)
    if not uuid then return -1 end
    if not state then
        state = {uuid = uuid, boss = e.self:GetID(), spawned = {}, electrify = os.time() + 20, venom = os.time() + 12}
        eq.set_timer(timer, 1000)
    end
    if state.uuid ~= uuid or state.boss ~= e.self:GetID() then return -1 end
    if not state.offspring then return raid.cap_damage(e, 45) end
    return 0
end

function M.tick(e)
    if e.timer ~= timer or not state or state.boss ~= e.self:GetID() then return end
    local boss, now = e.self, os.time()
    local top = boss:GetHateTop()
    local target = route.player(top)
    if not raid.member(target, state, binding) or target:GetHP() <= 0 then
        state.vacant = state.vacant or now
        if now - state.vacant >= 30 then
            raid.clear(state)
            state = nil
            boss:WipeHateList()
            boss:SetHP(boss:GetMaxHP())
            eq.stop_timer(timer)
        end
        return
    end
    state.vacant = nil

    if not state.offspring and boss:GetHP() <= math.floor(boss:GetMaxHP() * 45 / 100) then
        local adds = {}
        for _, point in ipairs(binding.offspring_pads) do
            local add = raid.spawn(state, layout.actors.offspring, point, top)
            if add then adds[#adds + 1] = add end
        end
        if #adds == 3 then
            state.offspring = true
            boss:Shout("My children, defend your lair!")
        else
            for _, add in ipairs(adds) do
                state.spawned[add:GetID()] = nil
                add:Depop(false)
            end
        end
    end
    if now >= state.electrify then
        if boss:CastSpell(layout.spells.electrify, top:GetID()) then state.electrify = now + 35 end
    elseif now >= state.venom then
        if boss:CastSpell(layout.spells.venom, top:GetID()) then state.venom = now + 22 end
    end
end

function M.death(e)
    if not state or not state.offspring or state.boss ~= e.killed_entity_id
        or not actor(e.killed) or not runtime.corpse(e) then return end
    if not raid.death_owner(e, state, binding) then return end
    local credited = raid.alive_members(binding, state.uuid)
    if raid.shared_step(credited, binding, 1) then
        raid.clear(state)
        state = nil
    end
end

return M
