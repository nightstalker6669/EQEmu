local M = {}
local task_id, journal_activity, journal_id, patroller_id = 920131, 1, 49186, 700142

function M.task_update(e)
    if e.task_id ~= task_id or e.activity_id ~= journal_activity or e.count ~= 1 then
        return
    end
    if not e.self or not e.self.valid or e.self:CountItem(journal_id) < 1 then
        return
    end
    -- EVENT_LOOT is a vetoable pre-loot event. This notification instead
    -- follows the inventory call and native journal objective credit.
    -- It is the ordinary native path, not a durable item-GUID receipt.
    local patroller = eq.spawn2(patroller_id, 0, 0,
        e.self:GetX(), e.self:GetY(), e.self:GetZ(), e.self:GetHeading())
    if patroller and patroller.valid then
        patroller:SetEntityVariable("rof2_spells_task_id", tostring(task_id))
        patroller:SetEntityVariable("rof2_spells_owner_character_id", tostring(e.self:CharacterID()))
        patroller:AddToHateList(e.self, 1)
    else
        eq.debug("Spells: Expedition patroller spawn failed after native journal task credit.")
        -- No guessed credit, refund, automatic spawn retry or reward. The
        -- original task accepts an ordinary patroller kill as well.
    end
end

return M
