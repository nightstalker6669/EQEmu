-- Explicit event bridge. Call from composed script adapters, never install by
-- mutating _G from require(). Existing handlers retain their return/veto.
local M={}
local function here()return eq.get_zone_id()==753 end
local function module(name)return require('achievement_quest_xorbb_'..name)end
function M.cast(e)
 if not here()then return end
 module('solution').cast(e);module('phials').cast(e);module('binding').cast(e)
 module('lightning').cast(e);module('eyes').cast(e);module('eyes').capture(e);module('golem').cast(e)
end
function M.validate(e)
 return module('mercenary_dialogue').validate(e)
end
function M.before_update(e)
 if not here()then return 0 end
 local r=module('mercenary').before_update(e)
 if r and r~=0 then return r end
 return module('krystuls').before_update(e)
end
function M.enter(e)
 if here()then module('gronk').player_enter(e.self);module('krystuls').enter(e)end
end
function M.closed(e)
 if not here()then return end
 local task=tonumber(e.task_id)
 if task==3040484 then module('eyes').clear(e.self)
 elseif task==3040485 then module('golem').clear(e.self)
 elseif task==3040486 then module('gronk').clear(e.self)end
 -- Keep monotonic run buckets: later repeats must have a new epoch.
end
function M.merit(e)
 if not here()then return end
 module('mercenary').merit(e);module('solution').merit(e);module('eyes').merit(e)
 module('gronk').merit(e);module('corpse').merit(e)
end
function M.spawn(e)
 if here()then module('krystuls').init(e.self)end
end
function M.click(e)
 if here()then module('krystuls').click(e)end
end
function M.timer(e)
 if not here()then return end
 module('solution').timer(e);module('eyes').timer(e);module('golem').timer(e)
 module('gronk').giver_timer(e);module('gronk').timer(e);module('krystuls').timer(e)
end
function M.death(e)
 if not here()then return end
 module('solution').death(e);module('corpse').death(e)
end
return M
