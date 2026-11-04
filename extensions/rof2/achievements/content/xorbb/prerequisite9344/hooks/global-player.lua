-- BEGIN ROF2 XORBB TASK READINESS
local xorbb_previous_before_update=event_task_before_update
function event_task_before_update(e)
 if xorbb_previous_before_update then
  local result=xorbb_previous_before_update(e)
  if result and result~=0 then return result end
 end
 return require('achievement_quest_xorbb').before_update(e)
end
-- END ROF2 XORBB TASK READINESS

-- BEGIN ROF2 XORBB GROUND PICKUP
local xorbb_previous_pickup=event_player_pickup
function event_player_pickup(e)
 if xorbb_previous_pickup then
  local result=xorbb_previous_pickup(e)
  if result and result~=0 then return result end
 end
 return require('achievement_quest_xorbb_ground').before_pickup(e)
end
local xorbb_previous_complete=event_task_complete
function event_task_complete(e)
 local old
 if xorbb_previous_complete then old=xorbb_previous_complete(e)end
 require('achievement_quest_xorbb').closed(e)
 return old
end
local xorbb_previous_fail=event_task_fail
function event_task_fail(e)
 local old
 if xorbb_previous_fail then old=xorbb_previous_fail(e)end
 require('achievement_quest_xorbb').closed(e)
 return old
end
-- END ROF2 XORBB GROUND PICKUP

-- BEGIN ROF2 XORBB PHYSICAL INTERACTIONS
local xorbb_previous_door=event_click_door
function event_click_door(e)
 if xorbb_previous_door then
  local result=xorbb_previous_door(e)
  if result and result~=0 then return result end
 end
 local result=require('achievement_quest_xorbb_interactions').door(e)
 if result and result~=0 then return result end
 return require('achievement_quest_xorbb_cooks').door(e)
end
local xorbb_previous_stage=event_task_stage_complete
function event_task_stage_complete(e)
 local result
 if xorbb_previous_stage then result=xorbb_previous_stage(e)end
 require('achievement_quest_xorbb_group').stage(e)
 require('achievement_quest_xorbb_interactions').stage(e)
 require('achievement_quest_xorbb_ladder').stage(e)
 require('achievement_quest_xorbb_bats').stage(e)
 return result
end
local xorbb_previous_player_accepted=event_task_accepted
function event_task_accepted(e)
 local result
 if xorbb_previous_player_accepted then result=xorbb_previous_player_accepted(e)end
 require('achievement_quest_xorbb_interactions').accepted(e)
 return result
end
local xorbb_previous_enter=event_enter_zone
function event_enter_zone(e)
 local result
 if xorbb_previous_enter then result=xorbb_previous_enter(e)end
 require('achievement_quest_xorbb_interactions').refresh(e)
 require('achievement_quest_xorbb_bats').enter(e)
 require('achievement_quest_xorbb_spirit').start(e.self)
 return result
end
local xorbb_previous_timer=event_timer
function event_timer(e)
 if require('achievement_quest_xorbb_bats').timer(e)then return end
 if require('achievement_quest_xorbb_interactions').timer(e)then return end
 if xorbb_previous_timer then return xorbb_previous_timer(e)end
end
-- END ROF2 XORBB PHYSICAL INTERACTIONS

-- BEGIN ROF2 XORBB OWNED LADDER
local xorbb_previous_item_click=event_item_click_client
function event_item_click_client(e)
 if xorbb_previous_item_click then
  local result=xorbb_previous_item_click(e)
  if result and result~=0 then return result end
 end
 return require('achievement_quest_xorbb_ladder').click(e)
end
local xorbb_previous_zone=event_zone
function event_zone(e)
 local result
 if xorbb_previous_zone then result=xorbb_previous_zone(e)end
 if result and result~=0 then return result end -- Preserve vetoed zoning.
 require('achievement_quest_xorbb_ladder').clear(e.self)
 require('achievement_quest_xorbb_bats').clear(e.self)
 require('achievement_quest_xorbb_crystals').clear(e.self)
 require('achievement_quest_xorbb_spirit').clear(e.self)
 require('achievement_quest_xorbb_shelters').clear(e.self)
 require('achievement_quest_xorbb_ground').clear(e.self)
 return result
end
local xorbb_previous_disconnect=event_disconnect
function event_disconnect(e)
 local result
 if xorbb_previous_disconnect then result=xorbb_previous_disconnect(e)end
 require('achievement_quest_xorbb_ladder').clear(e.self)
 require('achievement_quest_xorbb_bats').clear(e.self)
 require('achievement_quest_xorbb_crystals').clear(e.self)
 require('achievement_quest_xorbb_spirit').clear(e.self)
 require('achievement_quest_xorbb_shelters').clear(e.self)
 require('achievement_quest_xorbb_ground').clear(e.self)
 return result
end
-- END ROF2 XORBB OWNED LADDER

-- BEGIN ROF2 XORBB COMPLETED TOTEM CAST
local xorbb_previous_cast=event_cast
function event_cast(e)
 if xorbb_previous_cast then
  local result=xorbb_previous_cast(e)
  if result and result~=0 then return result end
 end
 require('achievement_quest_xorbb_crystals').cast(e)
 return require('achievement_quest_xorbb_prison').lens(e)
end
-- END ROF2 XORBB COMPLETED TOTEM CAST
