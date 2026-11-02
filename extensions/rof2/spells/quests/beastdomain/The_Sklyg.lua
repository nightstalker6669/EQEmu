-- VoA original physical Cantrip / Periapt tasks. See provenance/voa-tasks.json.
local cantrips = {920110, 920111, 920112, 920113, 920114}
local periapts = {920115, 920116, 920117, 920118, 920119}
function event_say(e)
    local message = e.message:lower()
    if message:find("periapt", 1, true) then
        eq.task_selector(periapts)
    elseif message:find("hail", 1, true) or message:find("cantrip", 1, true) then
        eq.task_selector(cantrips)
    end
end
-- No event_trade: native task delivery consumes exactly one credited item and
-- returns excess hand-ins. A scripted return_items handler would bypass that.
-- Language admission and learned-spell choices are enforced in the task engine.
