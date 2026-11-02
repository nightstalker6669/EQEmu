local journal_click_before=event_item_click_client
function event_item_click_client(e)
 local r;if journal_click_before then r=journal_click_before(e)end
 if r==nil or r==0 then require('achievement_quest_bixie_journal').open(e)end
 return r
end
local journal_validate_before=event_combine_validate
function event_combine_validate(e)
 local r;if journal_validate_before then r=journal_validate_before(e)end
 if r~=nil and r~=0 then return r end
 return require('achievement_quest_bixie_journal').validate(e)
end
