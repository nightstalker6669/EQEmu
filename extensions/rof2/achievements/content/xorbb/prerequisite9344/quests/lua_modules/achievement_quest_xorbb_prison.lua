-- Actual named encounters, owned skulls and authenticated completed lens casts.
-- Native combat/death owns kill credit. No GM kill, manual objective or free lens.
local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_prison_data')
local M={}
local NS='rof2.xorbb.prison.v1'
local function yes(v)return v==true or v==1 end
local function enabled()
 return eq.get_zone_id()==753 and D.enabled==true and D.ready[P.task]==true
  and P.profile_ready==true and P.geometry_ready==true
end
local function active(c,a)
 return c and c.valid and yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,a))
end
local function named(n)
 return n and n.valid and(n:GetNPCTypeID()==P.sviir or n:GetNPCTypeID()==P.syrkl)
end
local function epoch(n)
 local token=n:GetEntityVariable(NS..'.epoch')
 if token==''then
  token=n:GetID()..':'..os.time()..':'..math.random(1,2147483647)
  n:SetEntityVariable(NS..'.epoch',token)
  n:SetEntityVariable(NS..'.entity',tostring(n:GetID()))
 end
 return token
end
local function owned(n,b)
 return n and n.valid and n:GetNPCTypeID()==P.skull
  and n:GetEntityVariable(NS..'.parent')==b:GetEntityVariable(NS..'.entity')
  and n:GetEntityVariable(NS..'.epoch')~=''
  and n:GetEntityVariable(NS..'.epoch')==b:GetEntityVariable(NS..'.epoch')
end
local function skulls(b)
 local out={}
 for n in eq.get_entity_list():GetNPCList().entries do
  if owned(n,b)and n:GetHP()>0 then out[#out+1]=n end
 end
 return out
end
function M.clear(b)
 if not b or not b.valid or b:GetNPCTypeID()~=P.syrkl then return end
 for _,n in ipairs(skulls(b))do n:Depop(false)end
end
function M.spawn(e)
 local n=e.self
 if not enabled()or not named(n)then return end
 epoch(n)
 n:SetSpecialAbility(26,1) -- Belly casting, not blanket magic immunity.
 if n:GetNPCTypeID()==P.syrkl then
  n:SetSpecialAbility(1,1) -- Jan7,2013 report describes native summoning.
  eq.set_next_hp_event(80)
 end
end
function M.say(e)
 local n,c=e.self,e.other
 if not enabled()or not named(n)or not c or not c.valid then return end
 local stage=n:GetNPCTypeID()==P.sviir and 3 or 9
 if not active(c,stage)then return end
 epoch(n)
 -- Match native targeted SpeakWith interactions; defer hostility briefly so
 -- other group members can make their own genuine report interaction.
 n:SetEntityVariable(NS..'.challenger.'..c:CharacterID(),'1')
 if n:IsEngaged()then n:AddToHateList(c,1);return end
 if n:GetEntityVariable(NS..'.challenge_due')==''then
  n:Say('You seek the prisoners? Then face the power of Xorbb.')
  n:SetEntityVariable(NS..'.challenge_due',tostring(os.time()+P.hail_delay))
 end
 n:SetTimer(P.timer,P.tick)
end
function M.guide(e)
 local n,c=e.self,e.other
 if not enabled()or not n or not n.valid or not c or not c.valid
  or not tostring(e.message or ''):lower():find('hail',1,true)then return end
 local lines={
  [753204]={0,'Follow the canal north and east. Thorndig found more signs.'},
  [753203]={1,'The halflings marked their way along the walls. Find Threshrock to the north.'},
  [753243]={2,'The eye waits in the room beside this passage. Confront Sviir.'}
 }
 local r=lines[n:GetNPCTypeID()]
 if r and active(c,r[1])then n:Say(r[2])end
end
function M.combat(e)
 local n=e.self
 if not enabled()or not named(n)then return end
 epoch(n)
 if yes(e.joined)then
  if n:GetNPCTypeID()==P.syrkl and n:GetHPRatio()>=100 then
   for i=1,#P.waves do n:SetEntityVariable(NS..'.wave.'..i,'')end
   n:SetEntityVariable(NS..'.failed','');eq.set_next_hp_event(80)
  end
  n:SetEntityVariable(NS..'.spell_due',tostring(os.time()+P.stasis_interval))
  n:SetEntityVariable(NS..'.beam_due',tostring(os.time()+P.beam_interval))
  n:SetEntityVariable(NS..'.beam_fire','');n:SetEntityVariable(NS..'.beam_left','')
  n:SetTimer(P.timer,P.tick)
 else
  n:StopTimer(P.timer);n:SetEntityVariable(NS..'.challenge_due','')
  n:SetEntityVariable(NS..'.beam_fire','');n:SetEntityVariable(NS..'.beam_left','')
  M.clear(n)
 end
end
local function wave(b,i)
 b:SetEntityVariable(NS..'.wave.'..i,'pending')
 local token=epoch(b)
 for _=1,P.waves[i].count do
  -- The actual live named actor is the anchor; no teleport or guessed route.
  -- geometry_ready still requires verifying its floor and summon placement.
  local m=eq.spawn2(P.skull,0,0,b:GetX(),b:GetY(),b:GetZ(),b:GetHeading())
  if not m or not m.valid then return false end
  local n=m:CastToNPC()
  if not n or not n.valid or n:GetNPCTypeID()~=P.skull then return false end
  n:SetEntityVariable(NS..'.parent',b:GetEntityVariable(NS..'.entity'))
  n:SetEntityVariable(NS..'.epoch',token)
  n:SetEntityVariable(NS..'.expires',tostring(os.time()+P.lifetime))
  if not owned(n,b)then n:Depop(false);return false end
  n:SetSpecialAbility(13,1) -- Period skull mez-immunity report.
  n:SetTimer(P.timer,P.tick)
  local target=b:GetHateTop()
  if target and target.valid then n:AddToHateList(target,1)end
 end
 b:SetEntityVariable(NS..'.wave.'..i,'done')
 return true
end
function M.hp(e)
 local b=e.self
 if not enabled()or not b or not b.valid or b:GetNPCTypeID()~=P.syrkl
  or b:GetHP()<=0 or b:GetEntityVariable(NS..'.failed')~=''then return end
 epoch(b)
 local hp=b:GetHPRatio();local next_hp
 -- A large real damage hit can cross several thresholds before another HP
 -- packet. Spawn each crossed wave once, rather than losing later thresholds.
 for i,w in ipairs(P.waves)do
  if b:GetEntityVariable(NS..'.wave.'..i)==''then
   if hp<w.hp then
    if not wave(b,i)then
     b:SetEntityVariable(NS..'.failed','partial_wave');M.clear(b)
     eq.debug('Xorbb skull-wave creation failed; no automatic retry.');return
    end
   elseif not next_hp then next_hp=w.hp end
  end
 end
 eq.set_next_hp_event(next_hp or -1)
end
local function challenge(n)
 local due=tonumber(n:GetEntityVariable(NS..'.challenge_due'))
 if not due or os.time()<due then return end
 n:SetEntityVariable(NS..'.challenge_due','')
 local stage=n:GetNPCTypeID()==P.sviir and 3 or 9
 for c in eq.get_entity_list():GetClientList().entries do
  if c.valid then
   local key=NS..'.challenger.'..c:CharacterID()
   if n:GetEntityVariable(key)=='1'then
    n:SetEntityVariable(key,'')
    if active(c,stage)or active(c,stage+1)then n:AddToHateList(c,1)end
   end
  end
 end
end
local function beam_once(n,left)
 left=math.min(left,#skulls(n))
 local target=n:GetHateTop()
 if left>0 and target and target.valid and yes(n:CastSpell(33699,target:GetID()))then left=left-1 end
 n:SetEntityVariable(NS..'.beam_left',tostring(left))
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not enabled()or not named(n)then return end
 challenge(n)
 if not n:IsEngaged()or n:GetHP()<=0 or n:IsCasting()then return end
 local now=os.time()
 if n:GetNPCTypeID()==P.sviir then
  local due=tonumber(n:GetEntityVariable(NS..'.spell_due'))or now
  local target=n:GetHateRandom()
  if now>=due and target and target.valid and yes(n:CastSpell(33703,target:GetID()))then
   n:SetEntityVariable(NS..'.spell_due',tostring(now+P.stasis_interval))
  end
  return
 end
 if n:GetEntityVariable(NS..'.failed')~=''then return end
 local left=tonumber(n:GetEntityVariable(NS..'.beam_left'))or 0
 if left>0 then
  beam_once(n,left);return
 end
 local fire=tonumber(n:GetEntityVariable(NS..'.beam_fire'))
 if fire then
  if now>=fire then
   n:SetEntityVariable(NS..'.beam_fire','')
   n:SetEntityVariable(NS..'.beam_due',tostring(now+P.beam_interval))
   beam_once(n,#skulls(n))
  end
 elseif now>=(tonumber(n:GetEntityVariable(NS..'.beam_due'))or now)then
  n:Emote('draws power from its surviving skulls.')
  n:SetEntityVariable(NS..'.beam_fire',tostring(now+P.beam_warning))
 end
end
function M.skull_timer(e)
 local n=e.self
 if e.timer~=P.timer or not n or not n.valid or n:GetNPCTypeID()~=P.skull then return end
 local id=tonumber(n:GetEntityVariable(NS..'.parent'))
 local token=n:GetEntityVariable(NS..'.epoch')
 if not id or token==''then return end -- Untagged/foreign actors are untouched.
 local b=eq.get_entity_list():GetNPCByID(id)
 local expires=tonumber(n:GetEntityVariable(NS..'.expires'))
 if not b or not b.valid or b:GetNPCTypeID()~=P.syrkl or b:GetHP()<=0
  or b:GetEntityVariable(NS..'.epoch')~=token or not expires or os.time()>=expires then n:Depop(false)end
end
function M.lens(e)
 local c,t=e.self,e.target
 if tonumber(e.cast_item_id)~=66521 or not enabled()or not active(c,10)
  or tonumber(e.caster_id)~=c:GetID()or not yes(e.cast_item_charge_consumed)
  or not e.spell or not e.spell.valid or e.spell:ID()~=23998
  or not t or not t.valid or not t:IsNPC()then return end
 local n=t:CastToNPC()
 if n:GetNPCTypeID()~=P.skull or n:GetHP()<=0 or n:GetID()~=tonumber(e.target_id)
  or n:GetEntityVariable(NS..'.lens')~=''then return end
 local id=tonumber(n:GetEntityVariable(NS..'.parent'))
 local b=id and eq.get_entity_list():GetNPCByID(id)or nil
 if not b or not b.valid or b:GetNPCTypeID()~=P.syrkl or not owned(n,b)
  or b:GetHP()<=0 or not b:IsEngaged()then return end
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 if dx*dx+dy*dy>P.lens_xy^2 or math.abs(dz)>P.lens_z
  or not c:CheckLoS(n)or not c:IsAttackAllowed(n,true)then return end
 n:SetEntityVariable(NS..'.lens','pending')
 -- One consumed original lens, one normally attributed lethal damage attempt.
 -- No second inventory deletion, GM Kill, artificial corpse or task increment.
 n:Damage(c,math.max(1,n:GetHP()),23998,0,false)
end
function M.death(e)
 if e.self and e.self.valid and e.self:GetNPCTypeID()==P.syrkl then M.clear(e.self)end
end
return M
