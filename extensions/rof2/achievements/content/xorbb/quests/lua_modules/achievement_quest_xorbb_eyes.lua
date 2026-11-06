-- Original Lurr actions, exact owned replacement actors and native merit/custody.
-- No native acceptance yet. The original world's native Spawn2 timer is started
-- once at conversion; replacement teardown never controls that source again.
local D=require('achievement_quest_xorbb_mercenary_data')
local P=require('achievement_quest_xorbb_eyes_data')
local W=require('achievement_quest_xorbb_world_eye')
local I=require('achievement_quest_mesa_inventory')
local M={};local NS='rof2.xorbb.eyes.v1'
local function yes(v)return v==true or v==1 end
local function enabled()return D.enabled==true and D.ready[P.task]==true and P.profiles_ready==true and eq.get_zone_id()==753 end
local function active(c,a)return enabled()and c and c.valid and yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,a))end
local function run(c)return c:GetBucket('rof2.xorbb.mercenary.run.'..P.task)end
local function tagged(n)
 return n and n.valid and P.types[n:GetNPCTypeID()]and n:GetSpawnPointID()==0
  and n:GetEntityVariable(NS..'.token')~=''and n:GetEntityVariable(NS..'.owner')~=''
end
local function outputs(token)
 local r={}
 for n in eq.get_entity_list():GetNPCList().entries do
  if tagged(n)and n:GetEntityVariable(NS..'.token')==token then r[#r+1]=n end
 end
 return r
end
local function unlock(n,token)
 if n and n.valid and n:GetEntityVariable(W.lock_key)==token then n:SetEntityVariable(W.lock_key,'')end
end
local function remove(n)
 if tagged(n)and n:GetEntityVariable(NS..'.state')~='clearing'then
  n:SetEntityVariable(NS..'.state','clearing');n:Depop(false)
 end
end
local function clear(token,rollback_prepare)
 local wave=outputs(token)
 if rollback_prepare and #wave>0 then
  local only_prepare=true
  for _,n in ipairs(wave)do if n:GetEntityVariable(NS..'.state')~='prepare'then only_prepare=false end end
  if only_prepare then
   local n=wave[1];local source=eq.get_entity_list():GetNPCByID(tonumber(n:GetEntityVariable(NS..'.source')))
   if source and source.valid and source:GetNPCTypeID()==tonumber(n:GetEntityVariable(NS..'.source_type'))
    and source:GetSpawnPointID()==tonumber(n:GetEntityVariable(NS..'.source_spawn'))then unlock(source,token)end
  end
 end
 for _,n in ipairs(wave)do remove(n)end
end
local function owner(n)
 local id=tonumber(n:GetEntityVariable(NS..'.owner'))
 local c=id and eq.get_entity_list():GetClientByCharID(id)or nil
 if c and c.valid and run(c)==n:GetEntityVariable(NS..'.run')then return c end
end
local function begin(e,p)
 local c,n=e.self,e.target:CastToNPC();local epoch=run(c)
 if not tostring(epoch):match('^[1-9]%d*$')or not W.allowed(c,n,nil,160)then return end
 -- There may be surviving attackers from the same transformation. Do not make
 -- another wave for the same caster/run/stage while any exact owned output lives.
 for a in eq.get_entity_list():GetNPCList().entries do
  if tagged(a)and a:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())
   and a:GetEntityVariable(NS..'.run')==epoch
   and tonumber(a:GetEntityVariable(NS..'.activity'))==p.activity then return end
 end
 local token=NS..':'..c:CharacterID()..':'..epoch..':'..n:GetID()..':'..p.activity
 local count=p.count
 if p.npc==753911 and math.random()<P.irritated_two_probability then count=2 end
 local source_id,source_type,spawn=n:GetID(),n:GetNPCTypeID(),n:GetSpawnPointID()
 local x,y,z,h=n:GetX(),n:GetY(),n:GetZ(),n:GetHeading();local prepared={}
 n:SetEntityVariable(W.lock_key,token)
 for index=1,count do
  local m=eq.spawn2(p.npc,0,0,x,y,z,h)
  if not m or not m.valid or not m:IsNPC()then clear(token);unlock(n,token);return end
  local a=m:CastToNPC()
  if a:GetNPCTypeID()~=p.npc or a:GetSpawnPointID()~=0 then
   for _,known in ipairs(prepared)do known:SetEntityVariable(NS..'.state','quarantine')end
   eq.debug('Xorbb eye output identity unknown; source quarantined before conversion.');return
  end
  a:SetEntityVariable(NS..'.token',token);a:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()))
  a:SetEntityVariable(NS..'.run',epoch);a:SetEntityVariable(NS..'.activity',tostring(p.activity))
  a:SetEntityVariable(NS..'.next',tostring(p.next_activity));a:SetEntityVariable(NS..'.kill',tostring(p.kill_activity or -1))
  a:SetEntityVariable(NS..'.count',tostring(count));a:SetEntityVariable(NS..'.source',tostring(source_id))
  a:SetEntityVariable(NS..'.source_type',tostring(source_type));a:SetEntityVariable(NS..'.source_spawn',tostring(spawn))
  a:SetEntityVariable(NS..'.born',tostring(os.time()));a:SetEntityVariable(NS..'.state','prepare')
  a:SetInvul(true);a:SetTargetable(false);a:SetSpecialAbility(24,1);a:SetSpecialAbility(25,1)
  a:SetTimer(P.timer,P.interval_seconds);prepared[#prepared+1]=a
 end
 if #prepared~=count or not W.allowed(c,n,nil,160,token)then
  clear(token);unlock(n,token);return
 end
 -- Full health is provided by new native NPC construction, not a heal/reuse of
 -- the old eye. Source identity and ordinary respawn timing remain native.
 for _,a in ipairs(prepared)do
  if a:GetHP()<=0 or a:GetHP()~=a:GetMaxHP()then clear(token);unlock(n,token);return end
  a:SetEntityVariable(NS..'.state','committed')
 end
 n:Depop(true) -- exactly once; activation waits for exact source disappearance
end
function M.cast(e)
 local c,t=e.self,e.target;local p=P.actions[tonumber(e.cast_item_id)]
 if not p or not active(c,p.activity)or not c:IsClient()
  or tonumber(e.caster_id)~=c:GetID()or not yes(e.cast_item_charge_consumed)
  or not e.spell or not e.spell.valid or e.spell:ID()~=p.spell
  or not t or not t.valid or not t:IsNPC()or tonumber(e.target_id)~=t:GetID()then return end
 begin(e,p)
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not tagged(n)then return end
 local c=owner(n);local born=tonumber(n:GetEntityVariable(NS..'.born'))
 if not enabled()or not c or not yes(c:IsTaskActive(P.task))or not born
  or os.time()-born>=P.lifetime_seconds then clear(n:GetEntityVariable(NS..'.token'),true);return end
 local state=n:GetEntityVariable(NS..'.state')
 if state=='prepare'and os.time()-born>P.prepare_timeout_seconds then clear(n:GetEntityVariable(NS..'.token'),true);return end
 if state~='committed'then return end -- Active second attackers survive first kill.
 local token=n:GetEntityVariable(NS..'.token')
 local source=eq.get_entity_list():GetNPCByID(tonumber(n:GetEntityVariable(NS..'.source')))
 if source and source.valid and source:GetEntityVariable(W.lock_key)==token then
  if os.time()-born>P.prepare_timeout_seconds then clear(token);eq.debug('Xorbb source removal unconfirmed; no replay/credit.')end
  return
 end
 local wave=outputs(token);local expected=tonumber(n:GetEntityVariable(NS..'.count'))
 local a=tonumber(n:GetEntityVariable(NS..'.activity'));local next_a=tonumber(n:GetEntityVariable(NS..'.next'))
 if #wave~=expected or not active(c,a)then clear(token);return end
 for _,m in ipairs(wave)do
  if m:GetEntityVariable(NS..'.state')~='committed'or m:GetHP()<=0 then clear(token);return end
 end
 for _,m in ipairs(wave)do
  m:SetEntityVariable(NS..'.state','active');m:SetInvul(false);m:SetTargetable(true)
  m:SetSpecialAbility(24,0);m:SetSpecialAbility(25,0);m:AddToHateList(c,1)
 end
 c:UpdateTaskActivity(P.task,a,1)
 if not yes(c:IsTaskActivityActive(P.task,next_a))then eq.debug('Xorbb eye cast progress unconfirmed after activation; no automatic replay.')end
end
function M.merit(e)
 local n,c=e.self,e.other
 if not tagged(n)or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient();local a=tonumber(n:GetEntityVariable(NS..'.kill'))
 if not a or a<0 then return end
 if n:GetEntityVariable(NS..'.state')~='active'or n:GetEntityVariable(NS..'.credited')~=''
  or c:CharacterID()~=tonumber(n:GetEntityVariable(NS..'.owner'))
  or run(c)~=n:GetEntityVariable(NS..'.run')or not active(c,a)then return end
 n:SetEntityVariable(NS..'.credited','1');c:UpdateTaskActivity(P.task,a,1)
end
function M.capture(e)
 local c,t=e.self,e.target
 if tonumber(e.cast_item_id)~=68579 or not active(c,18)or not yes(e.cast_item_charge_consumed)
  or tonumber(e.caster_id)~=c:GetID()or not e.spell or not e.spell.valid or e.spell:ID()~=33990
  or not t or not t.valid or not t:IsNPC()or tonumber(e.target_id)~=t:GetID()then return end
 local n=t:CastToNPC()
 if not tagged(n)or n:GetNPCTypeID()~=753912 or n:GetEntityVariable(NS..'.state')~='active'
  or n:GetEntityVariable(NS..'.owner')~=tostring(c:CharacterID())or n:GetEntityVariable(NS..'.run')~=run(c)
  or n:GetHP()<=0 or n:GetEntityVariable(NS..'.capture')~=''then return end
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 if dx*dx+dy*dy+dz*dz>160^2 or not c:CheckLoS(n)or I.count(c,68614)~=0
  or c:GetInventory():GetItem(Slot.Cursor).valid then return end
 n:SetEntityVariable(NS..'.capture','pending')
 local before=I.count(c,68614);local given=I.give(c,68614,1);local after=I.count(c,68614)
 if not given or after~=before+1 then
  if after==before then n:SetEntityVariable(NS..'.capture','')else eq.debug('Runny eye custody ambiguous; no replay.')end
  return
 end
 n:SetEntityVariable(NS..'.capture','done');remove(n);c:UpdateTaskActivity(P.task,18,1)
end
function M.clear(c)
 if not c or not c.valid then return end
 local tokens={}
 for n in eq.get_entity_list():GetNPCList().entries do
  if tagged(n)and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())
   and n:GetEntityVariable(NS..'.run')==run(c)then tokens[n:GetEntityVariable(NS..'.token')]=true end
 end
 for token in pairs(tokens)do clear(token,true)end
end
return M
