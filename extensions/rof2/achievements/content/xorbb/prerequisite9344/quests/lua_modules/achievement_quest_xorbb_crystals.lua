local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_crystals_data')
local M={}
local NS='rof2.xorbb.crystal.v1'
local function yes(v)return v==true or v==1 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<math.huge end
local function enabled()
 return eq.get_zone_id()==753 and D.enabled==true and D.ready[3040475]==true
  and P.profile_ready==true and P.aura_profile_ready==true and P.mines_geometry_ready==true
end
local function active(c)
 return c and c.valid and yes(c:IsTaskActive(3040475))and yes(c:IsTaskActivityActive(3040475,9))
end
local function aura(n)
 return n and n.valid and n:GetNPCTypeID()==P.aura_npc
  and n:GetEntityVariable(NS..'.owner')~=''and n:GetEntityVariable(NS..'.expires')~=''
end
local function covered(n)
 for a in eq.get_entity_list():GetNPCList().entries do
  local expires=aura(a)and tonumber(a:GetEntityVariable(NS..'.expires'))or nil
  if expires and os.time()<expires then
   local dx,dy,dz=n:GetX()-a:GetX(),n:GetY()-a:GetY(),n:GetZ()-a:GetZ()
   if dx*dx+dy*dy<=P.radius_xy^2 and math.abs(dz)<=P.radius_z then return true end
  end
 end
 return false
end
function M.cast(e)
 local c=e.self
 -- EVENT_CAST's completed-item context is supplied only by successful native
 -- CastedSpellFinished. A right-click, interrupted cast or unrelated spell is
 -- never a substitute for this successful original-item action.
 if tonumber(e.cast_item_id)~=66518 or not enabled()or not active(c)
  or tonumber(e.caster_id)~=c:GetID()or not e.spell or not e.spell.valid
  or e.spell:ID()~=23998 then return end
 local slot=tonumber(e.cast_item_slot)
 if not slot or c:GetItemIDAt(slot)~=66518 then return end
 local x,y,z=c:GetX(),c:GetY(),c:GetZ()
 for _,k in ipairs({'min_x','max_x','min_y','max_y','min_z','max_z'})do
  if not finite(P[k])then return end
 end
 if not(x>=P.min_x and x<=P.max_x and y>=P.min_y and y<=P.max_y and z>=P.min_z and z<=P.max_z)then
  c:Message(15,'Use the visibility totem in the crystal mines.');return
 end
 local owner=tostring(c:CharacterID());local count=0
 for a in eq.get_entity_list():GetNPCList().entries do
  if aura(a)and a:GetEntityVariable(NS..'.owner')==owner then count=count+1 end
 end
 if count>=P.max_owned_auras then
  c:Message(15,'Your existing visibility totems must expire before placing another.');return
 end
 local m=eq.spawn2(P.aura_npc,0,0,x,y,z,c:GetHeading())
 if not m or not m.valid then c:Message(13,'The visibility totem could not be placed.');return end
 local a=m:CastToNPC()
 if not a or not a.valid or a:GetNPCTypeID()~=P.aura_npc then
  c:Message(13,'The visibility totem actor could not be identified.');return
 end
 a:SetEntityVariable(NS..'.owner',owner)
 a:SetEntityVariable(NS..'.expires',tostring(os.time()+P.lifetime))
 if not aura(a)then a:Depop(false);return end
 a:SetTargetable(false);a:SetInvul(true);a:SetTimer(P.timer,P.interval)
 -- No inventory grant, extra charge removal, kill or Loot credit here.
end
local function visibility(n,on)
 local state=on and 'visible'or'hidden'
 if n:GetEntityVariable(NS..'.state')==state then return end
 n:SetEntityVariable(NS..'.state',state)
 n:ChangeRace(on and P.visible_race or P.hidden_race)
 n:SetTargetable(on);n:SetInvul(not on)
 n:SetSpecialAbility(19,on and 0 or 1) -- Melee immunity outside an aura.
 n:SetSpecialAbility(20,on and 0 or 1) -- Magic immunity outside, NOT while visible.
 n:SetSpecialAbility(24,on and 0 or 1) -- No new aggression while hidden.
 n:SetSpecialAbility(25,on and 0 or 1) -- No social aggression while hidden.
 n:SetSpecialAbility(26,1) -- Period Mar1,2013 belly-casting report.
 n:SetEntityVariable(NS..'.head','0')
 n:SetEntityVariable(NS..'.next',tostring(os.time()+P.head_interval))
 if not on then
  n:InterruptSpell();n:WipeHateList();n:ResumeWandering()
 end
end
function M.spawn(e)
 local n=e.self
 if not enabled()or not n or not n.valid or n:GetNPCTypeID()~=P.npc then return end
 n:SetEntityVariable(NS..'.state','')
 visibility(n,covered(n));n:SetTimer(P.timer,P.interval)
end
function M.timer(e)
 if e.timer~=P.timer or not e.self or not e.self.valid then return end
 local n=e.self
 if aura(n)then
  local expires=tonumber(n:GetEntityVariable(NS..'.expires'))
  if not expires or os.time()>=expires then n:Depop(false)end
  return
 end
 if not enabled()or n:GetNPCTypeID()~=P.npc then return end
 local on=covered(n);visibility(n,on)
 if not on or not n:IsEngaged()or n:IsCasting()then return end
 local due=tonumber(n:GetEntityVariable(NS..'.next'))or os.time()
 if os.time()<due then return end
 local head=tonumber(n:GetEntityVariable(NS..'.head'))or 0
 if head<3 then
  head=head+1;n:Emote('channels energy through head '..head..'.')
 else
  -- Original PBAE spell handles damage and its linked fear; do not separately
  -- cast fear or fabricate an extra hit. Retry only the normal spell AI cycle.
  if not yes(n:CastSpell(33533,n:GetID()))then return end
  head=0
 end
 n:SetEntityVariable(NS..'.head',tostring(head))
 n:SetEntityVariable(NS..'.next',tostring(os.time()+P.head_interval))
end
function M.merit(e)
 local n,c=e.self,e.other
 if not enabled()or not n or not n.valid or n:GetNPCTypeID()~=P.npc
  or n:GetEntityVariable(NS..'.state')~='visible'or not covered(n)
  or not c or not c.valid or not c:IsClient()then return end
 if active(c:CastToClient())then n:SetEntityVariable(NS..'.eligible','1')end
end
function M.death(e)
 local n,corpse=e.self,e.corpse
 if not enabled()or not n or not n.valid or n:GetNPCTypeID()~=P.npc
  or n:GetEntityVariable(NS..'.eligible')~='1'or not corpse or not corpse.valid
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or corpse:GetID()~=e.killed_entity_id or n:GetEntityVariable(NS..'.supplied')~=''then return end
 if corpse:CountItem(66516)~=0 then
  n:SetEntityVariable(NS..'.supplied','other_producer');return
 end
 local count=math.random(P.min_facets,P.max_facets)
 n:SetEntityVariable(NS..'.supplied','pending')
 for _=1,count do corpse:AddItem(66516,1)end
 if corpse:CountItem(66516)==count then n:SetEntityVariable(NS..'.supplied','done')
 else eq.debug('Xorbb facet corpse supply needs review; no automatic retry.')end
end
function M.clear(c)
 if not c or not c.valid or eq.get_zone_id()~=753 then return end
 local owner=tostring(c:CharacterID())
 for n in eq.get_entity_list():GetNPCList().entries do
  if aura(n)and n:GetEntityVariable(NS..'.owner')==owner then n:Depop(false)end
 end
 -- Shared crystals and other players' aura actors are never depopped here.
end
function M.closed(e)
 if tonumber(e.task_id)==3040475 then M.clear(e.self)end
end
return M
