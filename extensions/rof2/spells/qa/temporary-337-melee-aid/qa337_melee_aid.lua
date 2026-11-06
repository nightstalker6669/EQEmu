-- Unregistered QA helper. No global/player hook, automatic cast or other target.
-- Runtime owner invokes only after exact spell/client freshness and authorization.
local M = {}
local SPELL = 39999
local baseline

local function permitted(client)
    assert(client and client.valid and client:CharacterID() == 337,
        "QA aid requires character 337")
    assert(client:GetGM() and client:GetLevel() == 100,
        "QA aid requires the retained level-100 GM test character")
end

local function ids(client)
    local result = {}
    for _, id in ipairs(client:GetBuffSpellIDs()) do result[#result+1] = id end
    return result
end

function M.apply(client, phase)
    permitted(client)
    assert(phase == "kill_objectives", "QA aid is forbidden in weakening/capture phases")
    assert(not baseline and not client:FindBuff(SPELL), "Existing/pending aid: reconcile; do not refresh")
    -- Native refusal on conflict/overwrite. Never remove an existing buff to fit.
    assert(client:CanBuffStack(SPELL, client:GetLevel(), true) >= 0,
        "QA aid cannot occupy a free slot without overwrite")
    baseline = ids(client)
    local ok, landed = pcall(function() return client:SpellFinished(SPELL, client) end)
    if not ok or not landed or not client:FindBuff(SPELL) then
        client:BuffFadeBySpellID(SPELL)
        error("QA aid application uncertain/refused: stop and retain original buff receipt")
    end
    -- Caller records this returned preimage durably in its existing tester receipt.
    -- No original spell is faded/recast and no persistent QA journal is introduced.
    return {character=337, spell_id=SPELL, original_buff_ids=baseline, current_buff_ids=ids(client)}
end

function M.remove(client, phase)
    permitted(client)
    -- Cleanup is allowed even after a script reload lost its transient baseline.
    -- Exact unique QA ID and character guard prevent any blanket buff removal.
    client:BuffFadeBySpellID(SPELL)
    assert(not client:FindBuff(SPELL), "QA aid remains: halt damage/capture and escalate")
    local receipt = {character=337, spell_id=SPELL, phase=phase, aid_absent=true,
        original_buff_ids=baseline, current_buff_ids=ids(client)}
    baseline = nil
    return receipt
end

return M
