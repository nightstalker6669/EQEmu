-- TEMPORARY QA337 command entry. No say hook, general evaluator, or global dispatch.
-- Remove this exact owned file after explicit aid removal and phase completion.
local function array_json(values)
    if values == nil then return "null" end
    local encoded = {}
    for _, id in ipairs(values) do
        assert(type(id) == "number" and id == math.floor(id) and id >= 0 and id <= 65535,
            "Unexpected native buff ID")
        encoded[#encoded+1] = tostring(id)
    end
    return "[" .. table.concat(encoded, ",") .. "]"
end

function event_command(e)
    -- Native exporter strips the leading #; args is a 1-based sparse table.
    local command = e.command
    if command ~= "aidapply" and command ~= "aidremove" then return 0 end
    local client = e.self
    if not client or not client.valid or client:CharacterID() ~= 337
        or not client:GetGM() or client:GetGMStatus() < 200 or client:GetLevel() ~= 100 then
        return 1
    end
    -- No user-supplied code, spell, character, target, phase, or extra argument.
    if type(e.args) ~= "table" or next(e.args) ~= nil then
        client:Message(13, 'QA337_AID {"character":337,"spell_id":39999,"status":"REFUSED_ARGUMENTS"}')
        return 1
    end
    local ok, receipt = pcall(function()
        local aid = require("qa337_melee_aid")
        if command == "aidapply" then
            return aid.apply(client, "kill_objectives")
        end
        return aid.remove(client, "phase_end")
    end)
    if not ok then
        client:Message(13, 'QA337_AID {"character":337,"spell_id":39999,"status":"ERROR_STOP_RECONCILE"}')
        return 1
    end
    local absent = receipt.aid_absent == true and "true" or "false"
    client:Message(15, 'QA337_AID {"character":337,"spell_id":39999,"command":"' .. command ..
        '","aid_absent":' .. absent .. ',"original_buff_ids":' .. array_json(receipt.original_buff_ids) ..
        ',"current_buff_ids":' .. array_json(receipt.current_buff_ids) .. '}')
    return 1
end
