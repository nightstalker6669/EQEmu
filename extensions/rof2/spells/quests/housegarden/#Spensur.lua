-- Original HoT physical-fragment exchange; task policy enforces faction.
-- Unverified level87/88 faction admission remains withheld; see provenance.
local tasks = {920125, 920128, 920129}
function event_say(e)
 local message = e.message:lower()
 if message:find("spell", 1, true) then
  e.self:Say("Spensur give you spells if you find him dreams.")
  eq.task_selector(tasks)
 elseif message:find("dream", 1, true) then
  e.self:Say("All the dreams. Dey gone! From my head! Spensurr have [spells] but no dreams.")
 elseif message:find("hail", 1, true) then
  e.self:Say("Hurr? Is you a [dream]?")
 end
end

-- No event_trade: native task delivery consumes the credited fragment and returns extras.
