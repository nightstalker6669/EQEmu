-- Original HoT physical-fragment exchange; task policy enforces faction.
local tasks = {920120, 920121, 920122, 920123, 920124}
function event_say(e)
 local message = e.message:lower()
 if message:find("spell", 1, true) then
  e.self:Say("If you bring me a nebulous dream fragment, I can weave a new experience for you.")
  eq.task_selector(tasks)
 elseif message:find("dream", 1, true) then
  e.self:Say("Well, dream fragments. Nebulous fragments, to be exact. They can be woven into a lesson, like an ability or a [spell].")
 elseif message:find("hail", 1, true) then
  e.self:Say("Greetings. Have you found any [dreams] recently?")
 end
end

-- No event_trade: native task delivery consumes the credited fragment and returns extras.
