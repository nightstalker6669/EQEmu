local contacts = require("rof2_tss_ashengate_contacts")

function event_say(e) contacts.say(e, "east") end
function event_task_accepted(e) contacts.accepted(e, "east") end
