local expedition = require("rof2_hot_expedition")

function event_task_update(e)
    -- Zone-local notification; QuestParserCollection also dispatches global
    -- player hooks. No global handler is replaced or called a second time.
    expedition.task_update(e)
    return 0
end
