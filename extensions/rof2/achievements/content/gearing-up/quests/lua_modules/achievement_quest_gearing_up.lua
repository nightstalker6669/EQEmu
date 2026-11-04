-- Gearing Up (period choice selector and one-active-family policy).
-- Configured policy and native task SQL are installed together; no scripted rewards.
local M={}
local FIRST,LAST,GIVER=3044001,3044008,436006
local function yes(v)return v==true or v==1 end
local function family(id)return type(id)=='number'and id>=FIRST and id<=LAST and id==math.floor(id)end
local function other_active(c,except)
 for id=FIRST,LAST do if id~=except and yes(c:IsTaskActive(id))then return id end end
end
function M.new(policy)
 assert(type(policy)=='table'and type(policy.admit)=='function','explicit reviewed admission policy required')
 local H={}
 function H.say(e)
  if not e.other or not e.other.valid or e.self:GetNPCTypeID()~=GIVER then return false end
  local message=(e.message or ''):lower()
  if message:find('hail',1,true)then
   if yes(policy.admit(e.other))then
    e.self:Say('We need recovered clockwork parts. If you are '..eq.say_link('brave enough',false,'brave enough')..', I can show you the work.')
   end
   return false -- Preserve a composed Screw Loose/Oculus hail, if installed.
  end
  if not message:find('brave enough',1,true)then return false end
  if other_active(e.other)then e.other:Message(13,'You are already working on a Gearing Up task.');return true end
  if not yes(policy.admit(e.other))then return true end
  local choices={}
  for id=FIRST,LAST do if yes(eq.is_task_appropriate(id))then choices[#choices+1]=id end end
  if #choices>0 then eq.task_selector(choices)end
  return true
 end
 function H.accepted(e)
  if not family(e.task_id)or not e.other or not e.other.valid then return false end
  -- Native EVENT_TASK_ACCEPTED occurs after assignment; its return is no veto.
  if e.self:GetNPCTypeID()~=GIVER or not yes(policy.admit(e.other))or other_active(e.other,e.task_id)then
   if yes(e.other:IsTaskActive(e.task_id))then e.other:FailTask(e.task_id)end
   e.other:Message(13,'You may work on only one eligible Gearing Up task at a time.')
  end
  return true
 end
 return H
end
return M
