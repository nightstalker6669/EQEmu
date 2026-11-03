-- ScriptFileID20970 is shared by exactly nine native Sentry Shards.
-- Item-click is needed for script-only Three66791 (no click spell).
assert(event_item_click==nil,'Sentry Shards have an unreviewed item-click owner')
local learner=require('achievement_god_learner')
function event_item_click(e)
 learner.read_output(e)
 -- Native parser ignores return for this pre-cast notification.
 return 0
end
