function event_say(e)
  require('rof2_sod_kithicor').say(e, 456171)
end

function event_task_accepted(e)
  require("rof2_sod_kithicor_personal").accepted(e)
end
