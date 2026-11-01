-- Rain of Fear: physical mote delivery tasks with native selectable rewards.
-- Reward bundles: Allakhazam quests 6004-6013; see the accompanying manifest.
local dreadmote_tasks = {920100, 920101, 920102, 920103, 920104}
local terrormote_tasks = {920105, 920106, 920107, 920108, 920109}

function event_say(e)
    local message = e.message:lower()
    if message:find("terrormote", 1, true) then
        eq.task_selector(terrormote_tasks)
    elseif message:find("dreadmote", 1, true) then
        eq.task_selector(dreadmote_tasks)
    elseif message:find("hail", 1, true) then
        e.self:Say("Have you brought a [Dreadmote] or a [Terrormote]?")
    end
end

-- No event_trade: Client::FinishTrade calls UpdateTasksOnDeliver first,
-- consumes exactly the credited quantity via RemoveTaskDeliveredItems,
-- and returns the remainder using the server's AlwaysReturnHandins rule.
-- A generic return_items trade handler would bypass that native consumption
-- branch and could return an already-credited mote. Do not add one here.
-- The task engine creates and persists the reward choice; this script does
-- not summon scrolls, award currency, or manually complete tasks.
