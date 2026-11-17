-- New numeric dispatcher; exact NPC-owner quest preimage is absent.
function event_say(e)
 require('rof2_sod_fieldofscale').say(e,'Jaled',452147)
end
function event_task_accepted(e)
 require('rof2_sod_fieldofscale').accepted(e)
end
