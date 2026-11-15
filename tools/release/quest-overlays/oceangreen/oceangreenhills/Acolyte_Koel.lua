-- Original hail entry through the actual admitted Hiran mission/DZ.
-- No task creation, guessed instance, fallback coordinates or direct credit.
function event_say(e)
  if e.message:lower():find("hail", 1, true) then
    if e.other:SodHiranTravel(e.self, false) then
      e.self:Say("Good luck to you!")
    end
  end
end
