-- Requires the matching native Origin-only durable receipt implementation.
-- No generic AA grant or void Save/SaveAA acknowledgement is substituted.
local M = {}
local FIRST_RANK_ID = 1000

function M.reconcile(client)
    if client:GetLevel() < 5 or client:GetAA(FIRST_RANK_ID) ~= 0 then return false end
    -- The native bool is true only after committed Origin1000 rank1/charges0
    -- readback, native eligibility, and post-commit memory publication.
    if not client:ReconcileOriginEntitlement() or client:GetAA(FIRST_RANK_ID) ~= 1 then
        return false
    end
    eq.popup("", "<c \"#F0F000\">You have been granted Origin.</c><br><br>Origin allows you to teleport back to your starting city.<br><br>Open Alternate Advancement with 'V', select Origin in the General tab, and press Hotkey to create its button.")
    return true
end

return M
