-- Period requester phrases and faction necklace acquisition. Native task
-- acceptance owns prerequisites, levels, enabled definitions and cooldowns.
-- No hail grants progression, currency or mission victory.
local M = {}
M.tasks = {
  Rain = 920191, Safe = 920192, Bitter = 920193,
  Charges = 920194, Vengeance = 920195, Impostor = 920196,
}

local function enabled()
  local value = eq.get_rule("SodProgression:EnableKithicorSlice"):lower()
  return value == "true" or value == "1"
end

local function phrase(message, text)
  return (message or ""):lower():find(text, 1, true) ~= nil
end

function M.scout(e, expected)
  if not enabled() or e.self:GetNPCTypeID() ~= expected then return end
  local light = expected == 456002
  if not light and expected ~= 456003 then return end
  if phrase(e.message, "hail") then
    e.self:Say(light and "If you [will] serve the Army of Light, I can give you a pendant of recognition."
      or "If you [need] recognition among the Army of Obliteration, tell me now.")
    return
  end
  if not phrase(e.message, light and "will" or "need") then return end
  local item = light and 41118 or 41119
  -- Recognition items are freely replaceable quest items, not earned theme
  -- flags. CountItem includes the cursor; do not pile up repeated hail grants.
  if e.other:CountItem(item) > 0 then
    e.self:Say("Equip the recognition I gave you when you approach our army's officers.")
    return
  end
  e.other:SummonItem(item)
  e.self:Say("Wear this recognition when you approach our army's officers.")
end

function M.say(e, expected)
  if not enabled() or e.self:GetNPCTypeID() ~= expected then return end
  if phrase(e.message, "hail") then
    -- Native SpeakWith processing follows the real NPC event.
    if expected == 456172 then
      e.self:Say("Will you [attend] to the troubling reports about Ladon Gendorin?")
    elseif expected == 456171 then
      e.self:Say("Will you [accept] the task concerning Ladon, or [volunteer] to confront the threat in the Commonlands?")
    elseif expected == 456174 then
      e.self:Say("We need help to [escort] the refugees to safety.")
    elseif expected == 456175 then
      e.self:Say("Will you perform the [deed] of confronting the impostor?")
    elseif expected == 456178 then
      e.self:Say("Are you ready for the [undertaking] against the enemy's supplies?")
    end
    return
  end
  if expected == 456172 and phrase(e.message, "attend") then
    eq.task_set_selector(6)
  elseif expected == 456174 and phrase(e.message, "escort") then
    eq.task_set_selector(4)
  elseif expected == 456171 and phrase(e.message, "volunteer") then
    eq.task_set_selector(5)
  elseif expected == 456171 and phrase(e.message, "accept") then
    -- A native set containing shared AND personal tasks is dispatched as a
    -- shared selector. Keep the personal continuation on its native selector;
    -- set5 groups the shared mission offered by this same provider.
    e.other:TaskSelectorFromNPC(e.self, { M.tasks.Vengeance })
  elseif expected == 456175 and phrase(e.message, "deed") then
    eq.task_set_selector(7)
  elseif expected == 456178 and phrase(e.message, "undertaking") then
    eq.task_set_selector(3)
  end
end

function M.travel(c,npc,return_to_camp)
  if not enabled() then return false end
  if c:SodKithicorTravel(npc,return_to_camp) then return true end
  c:SetEntityVariable('kithicor_travel',npc:GetID()..':'..(return_to_camp and 'return' or 'entry')..':'..(os.time()+8))
  eq.set_timer('kithicor_travel',500,c)
  return false
end

function M.fathim(e)
  if not enabled() or e.self:GetNPCTypeID()~=456163 then return end
  if phrase(e.message,'hail') then
    e.self:Say('Barvian told me to expect you. Tell me when you are [ready] to go.')
  elseif phrase(e.message,'ready') then
    M.travel(e.other,e.self,false)
  end
end

function M.book(e)
  if not enabled() or e.door:GetDoorID()~=19 then return end
  if e.self:SodKithicorBook(19) then return end
  e.self:SetEntityVariable('kithicor_travel','19:book:'..(os.time()+8))
  eq.set_timer('kithicor_travel',500,e.self)
end

function M.timer(e)
  if e.timer~='kithicor_travel' then return end
  local c=e.self
  local id,mode,expires=c:GetEntityVariable('kithicor_travel'):match('^(%d+):(%a+):(%d+)$')
  if not id or os.time()>tonumber(expires) then
    eq.stop_timer('kithicor_travel',c)
    c:SetEntityVariable('kithicor_travel','')
    return
  end
  local moved=false
  if mode=='book' then moved=c:SodKithicorBook(19)
  else
    local npc=eq.get_entity_list():GetNPCByID(tonumber(id))
    if npc.valid then moved=c:SodKithicorTravel(npc,mode=='return') end
  end
  if moved then
    eq.stop_timer('kithicor_travel',c)
    c:SetEntityVariable('kithicor_travel','')
  end
end

return M
