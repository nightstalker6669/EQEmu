-- Unregistered QA helper. No global/player hook, automatic cast or other target.
-- Runtime owner invokes only after exact spell/client freshness and authorization.
local M = {}
local SPELL = 39999
local baseline

local function permitted(client)
    assert(client and client.valid and client:IsClient() and client:CharacterID() == 337,
        "QA aid requires character 337")
    assert(client:GetGM() and client:GetGMStatus() >= 200 and client:GetLevel() == 100,
        "QA aid requires the retained level-100 GM test character")
end

local function stopped(client)
    assert(not client:IsAutoAttackEnabled() and not client:IsAutoFireEnabled(),
        "Stop native autoattack/autofire before changing the QA aid")
    -- Stick/nav controls are client-owner inputs, not claimed by this native check.
end

local function ids(client)
    local result = {}
    for _, id in ipairs(client:GetBuffSpellIDs()) do result[#result+1] = id end
    return result
end

function M.apply(client, phase)
    permitted(client)
    stopped(client)
    assert(phase == "kill_objectives" or phase == "weakening", "Unsupported QA phase")
    assert(client:GetItemIDAt(13) == 147495, "QA damage requires the retained primary mallet")
    local definition = eq.get_spell(SPELL)
    local required_base = phase == "weakening" and 100000 or 2000000
    assert(definition and definition.valid and definition:EffectID(11) == 220
        and definition:Base(11) == required_base and definition:Base2(11) == 0
        and definition:Formula(11) == 100 and definition:Max(11) == 0,
        "Wrong native QA damage profile: stop")
    assert(definition:TargetType() == 6 and definition:GoodEffect() == 1
        and definition:BuffdurationFormula() == 7 and definition:BuffDuration() == 10
        and definition:Range() == 0 and definition:AoeRange() == 0
        and definition:Mana() == 0 and definition:CastTime() == 0
        and definition:PersistDeath() == false, "Wrong native QA self-buff shape: stop")
    for slot=0,10 do
        assert(definition:EffectID(slot) == 254 and definition:Base(slot) == 0
            and definition:Base2(slot) == 0 and definition:Max(slot) == 0
            and definition:Formula(slot) == 100, "Unexpected extra QA effect")
    end
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
    return {character=337, spell_id=SPELL, phase=phase, damage_base=required_base,
        attack_stopped=true, auto_fire_stopped=true,
        original_buff_ids=baseline, current_buff_ids=ids(client)}
end

function M.remove(client, phase)
    permitted(client)
    stopped(client)
    -- Cleanup is allowed even after a script reload lost its transient baseline.
    -- Exact unique QA ID and character guard prevent any blanket buff removal.
    client:BuffFadeBySpellID(SPELL)
    assert(not client:FindBuff(SPELL), "QA aid remains: halt damage/capture and escalate")
    local receipt = {character=337, spell_id=SPELL, phase=phase, aid_absent=true,
        attack_stopped=true, auto_fire_stopped=true,
        original_buff_ids=baseline, current_buff_ids=ids(client)}
    baseline = nil
    return receipt
end

return M
