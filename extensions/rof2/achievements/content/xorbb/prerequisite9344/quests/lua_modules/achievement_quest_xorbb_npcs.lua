-- Authored NPC guidance plus the researched Gerbril hail/hostility/spell flow.
-- Deliver item consumption and unused-item/coin return remain native C++ owned.
local D=require('achievement_quest_xorbb_data')
local areas=require('achievement_quest_xorbb_phase_data')
local M={}
local function yes(v)return v==true or v==1 end
local function active(c,task,a)
 return D.enabled==true and D.ready[task]==true and eq.get_zone_id()==753 and c and c.valid
  and yes(c:IsTaskActive(task))and yes(c:IsTaskActivityActive(task,a))
end
local function inside(n,a)
 return n:GetX()>=a.min_x and n:GetX()<=a.max_x and n:GetY()>=a.min_y and n:GetY()<=a.max_y
  and n:GetZ()>=a.min_z and n:GetZ()<=a.max_z
end
local lines={
 [753095]={
  {3040475,4,'You found us. Give Barleyburr the ham, then bring me the poultice.'},
  {3040475,7,'We need a way past the eyes. Ask Thistlebrush about the crystals.'},
  {3040475,13,'We have the rings. Tell Thistlebrush we are ready.'},
  {3040476,6,'Xorbb\'s servants caught us. Speak with Barleyburr.'},
  {3040476,13,'Take word of our escape to Thistlebrush.'}
 },
 [753091]={
  {3040475,5,'The ham first, please.'},
  {3040475,12,'Bring the second ring here.'},
  {3040476,7,'The eyes keep watch. Gather lenses from them.'},
  {3040476,12,'The guard is gone. Speak with Brantun.'}
 }
}
function M.say(e)
 local n,c=e.self,e.other
 if not n or not n.valid or not c or not c.valid then return false end
 if n:GetNPCTypeID()==D.gerbril.npc and active(c,3040474,14)then
  -- The native SpeakWith producer accepts targeted NPC say interactions. Keep
  -- hostility coupled to that same interaction rather than only its Hail text.
  n:Say('You came for the halflings? Then you can take their place!')
  n:AddToHateList(c,1)
  return true
 end
 if not tostring(e.message or ''):lower():find('hail',1,true)then return false end
 if(n:GetNPCTypeID()==753095 or n:GetNPCTypeID()==753091)and active(c,3040476,11)then
  local a=areas[3040476]and areas[3040476][6]
  if a and inside(c,a)and inside(n,a)then
   require('achievement_quest_xorbb_rescue').claim(c)
   return true
  end
 end
 for _,r in ipairs(lines[n:GetNPCTypeID()]or{})do
  local a=areas[r[1]]and areas[r[1]][r[2]]
  if a and active(c,r[1],r[2])and inside(c,a)and inside(n,a)then
   if r[1]==3040476 and(r[2]==12 or r[2]==13)
    and not require('achievement_quest_xorbb_rescue').ring(c,r[2])then return true end
   n:Say(r[3]);return true
  end
 end
 return false
end
local function gerbril(n)
 return D.enabled==true and D.ready[3040474]==true and eq.get_zone_id()==753 and n and n.valid
  and n:GetNPCTypeID()==D.gerbril.npc
end
function M.combat(e)
 if not gerbril(e.self)then return end
 if yes(e.joined)then
  e.self:SetEntityVariable('xorbb.gerbril.next_spell','1')
  e.self:SetTimer(D.gerbril.timer,D.gerbril.spell_interval)
 else e.self:StopTimer(D.gerbril.timer)end
end
function M.timer(e)
 if e.timer~=D.gerbril.timer or not gerbril(e.self)then return false end
 local n=e.self
 if not n:IsEngaged()or n:GetHP()<=0 then n:StopTimer(e.timer);return true end
 if n:IsCasting()then return true end
 local target=n:GetHateRandom()
 if not target or not target.valid then return true end
 local which=tonumber(n:GetEntityVariable('xorbb.gerbril.next_spell'))or 1
 if which~=1 and which~=2 then return true end
 if n:CastSpell(D.gerbril.spells[which],target:GetID())then
  n:SetEntityVariable('xorbb.gerbril.next_spell',which==1 and'2'or'1')
 end
 return true
end
return M
