-- Selected composition of current item/AA data; not recovered original scripts.
local M = {}
local books = {
    [64554] = { ability = 708, level = 90 },
    [64555] = { ability = 709, level = 91 },
    [76276] = { ability = 1800, level = 95 },
    [76277] = { ability = 1801, level = 96 },
    [81136] = { ability = 754, level = 80 },
    [81137] = { ability = 755, level = 81 }
}

local function same_stack(owner, slot, item_id, serial)
    local item = owner:GetInventory():GetItem(slot)
    return item.valid and item:GetID() == item_id
        and item:GetSerialNumber() == serial and item:IsStackable()
        and item:GetCharges() > 0
end

function M.click_cast(e, expected_item_id)
    if not e.self.valid then
        return 0
    end
    local item_id = e.self:GetID()
    local book = books[item_id]
    if item_id ~= expected_item_id or not book then
        return 0
    end

    local owner = e.owner
    local slot = e.slot_id
    local serial = e.self:GetSerialNumber()
    if owner:GetClass() ~= 14 or owner:GetLevel() < book.level then
        owner:Message(13, "You must be an enchanter of at least level " .. book.level .. " to learn this ability.")
        return 1
    end
    if owner:GetAAByAAID(book.ability) ~= 0 then
        owner:Message(13, "You have already learned this ability. Your book has been retained.")
        return 1
    end
    if not same_stack(owner, slot, item_id, serial) then
        owner:Message(13, "The ability book could not be verified. Your stack has been retained.")
        return 1
    end

    -- Preserve native eligibility and payment checks, including the zero cost.
    local granted = owner:GrantAlternateAdvancementAbility(book.ability, 1, false)
    if not granted or owner:GetAAByAAID(book.ability) ~= 1 then
        owner:Message(13, "Learning the ability was not confirmed. Your book has been retained; contact a GM if your abilities changed.")
        return 1
    end
    -- Recheck the original instance after synchronous AA purchase callbacks.
    if not same_stack(owner, slot, item_id, serial) then
        owner:Message(13, "The ability was learned, but the original book stack moved. Contact a GM; do not use another book for this ability.")
        return 1
    end

    owner:DeleteItemInInventory(slot, 1, true)
    -- Use a fresh inventory lookup; never access the old item after deletion.
    -- A remaining stack reaches native veto recovery. An empty modern slot
    -- returns early, so supply its interrupt and spell-bar recovery here.
    if not owner:GetInventory():GetItem(slot).valid then
        owner:InterruptSpell(11370)
    end
    owner:Message(15, "You have learned the enchantment ability.")
    return 1
end

return M
