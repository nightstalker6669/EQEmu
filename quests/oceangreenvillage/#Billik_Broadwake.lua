-- The native SpeakWith pipeline handles Billik's objective updates.
-- The survival controller is still held; this script grants no victory.
function event_say(e)
  local scene=require('rof2_sod_scene')
  scene.enter(e.other)
  if e.message:lower():find('hail',1,true)then scene.start(e.other,e.self)end
  if e.message:lower():find("keep", 1, true) then
    if e.other:SodHiranTravel(e.self, true) then
      e.self:Say("Safe travels.")
    end
  end
end
