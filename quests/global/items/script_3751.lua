-- Tinkering Mastery books: native paid acquisition of ranks 1 through 3.
local books = {
    [98471] = { rank = 1, cost = 3 },
    [98472] = { rank = 2, cost = 6 },
    [98473] = { rank = 3, cost = 9 }
}

local function same_book(owner, slot, item_id, serial)
    local item = owner:GetInventory():GetItem(slot)
    return item.valid and item:GetID() == item_id
        and item:GetSerialNumber() == serial and not item:IsStackable()
end

function event_item_click_cast(e)
    if not e.self.valid then
        return 0
    end
    local item_id = e.self:GetID()
    local book = books[item_id]
    if not book then
        return 0
    end

    local owner = e.owner
    local slot = e.slot_id
    local serial = e.self:GetSerialNumber()
    if owner:GetBaseRace() ~= 12 or owner:GetLevel() < 59 then
        owner:Message(13, "You must be a level 59 gnome to learn Tinkering Mastery.")
        return 1
    end
    if owner:GetAAByAAID(575) ~= book.rank - 1 then
        owner:Message(13, "You must learn the Tinkering Mastery books in order, once each.")
        return 1
    end
    if owner:GetAAPoints() < book.cost then
        owner:Message(13, "You need " .. book.cost .. " unspent ability points to learn this rank.")
        return 1
    end
    if not same_book(owner, slot, item_id, serial) then
        owner:Message(13, "The Tinkering Mastery book could not be verified. Please try again.")
        return 1
    end

    -- false charges the native rank cost; never subtract AA points separately.
    local granted = owner:GrantAlternateAdvancementAbility(575, book.rank, false)
    if not granted or owner:GetAAByAAID(575) ~= book.rank then
        owner:Message(13, "Tinkering Mastery was not confirmed. Your book has been retained; contact a GM if your ability points changed.")
        return 1
    end
    -- AA purchase callbacks may change inventory. Recheck scalar identity.
    if not same_book(owner, slot, item_id, serial) then
        owner:Message(13, "Tinkering Mastery was learned, but the original book moved. Contact a GM; do not use another book for this rank.")
        return 1
    end
    -- Quantity zero removes this whole nonstackable instance. Quantity one
    -- would merely reduce charges on these unlimited-charge books.
    owner:DeleteItemInInventory(slot, 0, true)
    owner:InterruptSpell(11370)
    owner:Message(15, "You have learned Tinkering Mastery rank " .. book.rank .. ".")
    return 1
end
