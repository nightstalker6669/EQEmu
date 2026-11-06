-- Append once to the reviewed global_player script; never replace its existing body.
-- Installer must compare the complete predecessor hash and marker absence.
-- BEGIN ROF2 XORBB MERCENARY global_player
local xorbb_bridge=require('achievement_quest_xorbb_mercenary_hooks')
local xorbb_previous_event_cast=event_cast
function event_cast(e)
 local old
 if xorbb_previous_event_cast then old=xorbb_previous_event_cast(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.cast(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_task_before_update=event_task_before_update
function event_task_before_update(e)
 local old
 if xorbb_previous_event_task_before_update then old=xorbb_previous_event_task_before_update(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.before_update(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_enter_zone=event_enter_zone
function event_enter_zone(e)
 local old
 if xorbb_previous_event_enter_zone then old=xorbb_previous_event_enter_zone(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.enter(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_task_complete=event_task_complete
function event_task_complete(e)
 local old
 if xorbb_previous_event_task_complete then old=xorbb_previous_event_task_complete(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.closed(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_task_fail=event_task_fail
function event_task_fail(e)
 local old
 if xorbb_previous_event_task_fail then old=xorbb_previous_event_task_fail(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.closed(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_click_object=event_click_object
function event_click_object(e)
 local old
 if xorbb_previous_event_click_object then old=xorbb_previous_event_click_object(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.click(e)
 if result~=nil then return result end
 return old
end
local xorbb_previous_event_combine_validate=event_combine_validate
function event_combine_validate(e)
 local old
 if xorbb_previous_event_combine_validate then old=xorbb_previous_event_combine_validate(e);if old and old~=0 then return old end end
 local result=xorbb_bridge.validate(e)
 if result~=nil then return result end
 return old
end
-- END ROF2 XORBB MERCENARY global_player
