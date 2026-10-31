-- Register one extra option in the existing crossroads selector.
local sunderock_previous_board_click=event_click_door
function event_click_door(e)
 require('achievement_quest_sunderock').prepare_board(e)
 if sunderock_previous_board_click then return sunderock_previous_board_click(e)end
end
