local books = require("aa_enchantment_books")

function event_item_click_cast(e)
    return books.click_cast(e, 45975)
end
