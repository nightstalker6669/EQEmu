-- Researched Goblin ready -> owned demonstration -> actual now-window item use.
-- Real preceding task delivery owns68484 consumption; never consume it again.
local D=require('achievement_quest_xorbb_mercenary_data')
local S=require('achievement_quest_xorbb_scenes_data')
local P=S.golem
local G=require('achievement_quest_xorbb_scene_geometry')
local Supplies=require('achievement_quest_xorbb_mercenary_supplies')
local M={};local NS='rof2.xorbb.golem.v1'
local function yes(v)return v==true or v==1 end
local function enabled()return D.enabled==true and D.ready[P.task]==true and S.profiles_ready==true and eq.get_zone_id()==753 end
local function active(c,a)return enabled()and c and c.valid and yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,a))end
local function run(c)return c:GetBucket('rof2.xorbb.mercenary.run.'..P.task)end
local function tagged(n)return n and n.valid and n:GetNPCTypeID()==P.npc and n:GetSpawnPointID()==0
 and n:GetEntityVariable(NS..'.owner')~=''and n:GetEntityVariable(NS..'.run')~=''end
local function remove(n)
 if tagged(n)and n:GetEntityVariable(NS..'.state')~='clearing'then n:SetEntityVariable(NS..'.state','clearing');n:Depop(false)end
end
local function nearby(c,n)
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 return dx*dx+dy*dy<=P.owner_xy^2 and math.abs(dz)<=P.owner_z and c:CheckLoS(n)
end
local function find(c)
 local result
 for n in eq.get_entity_list():GetNPCList().entries do
  if tagged(n)and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())and n:GetEntityVariable(NS..'.run')==run(c)then
   if result then return nil,'multiple owned demonstration actors'end;result=n
  end
 end
 return result
end
function M.say(e)
 local n,c=e.self,e.other
 if not n or not n.valid or n:GetNPCTypeID()~=P.giver or not c or not c.valid
  or tonumber(e.language)~=14 or not tostring(e.message or ''):lower():find('%f[%a]ready%f[%A]')
  or not(active(c,10)or active(c,11))then return false end
 local epoch=run(c)
 if not tostring(epoch):match('^[1-9]%d*$')or not nearby(c,n)then return true end
 local existing,err=find(c)
 if err then c:Message(13,'The demonstration needs review before it can restart.');return true end
 if existing then
  if existing:GetEntityVariable(NS..'.state')=='waiting_ready'then
   existing:SetEntityVariable(NS..'.state','instructions');existing:SetEntityVariable(NS..'.cue',tostring(os.time()+P.cue_delay_seconds))
   Supplies.recover(c,P.giver)
  end
  return true
 end
 local points,why=G.points(n,P.offsets,P.geometry)
 if not points then eq.debug('Grolak placement refused: '..tostring(why));c:Message(13,'Grolak cannot prepare the demonstration here right now.');return true end
 local p=points[1];local m=eq.spawn2(P.npc,0,0,p.x,p.y,p.z,p.heading)
 if not m or not m.valid or not m:IsNPC()then return true end
 local a=m:CastToNPC()
 if a:GetNPCTypeID()~=P.npc or a:GetSpawnPointID()~=0 then eq.debug('Golem output identity unknown; no progress.');return true end
 a:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()));a:SetEntityVariable(NS..'.run',epoch)
 a:SetEntityVariable(NS..'.giver',tostring(n:GetID()));a:SetEntityVariable(NS..'.born',tostring(os.time()))
 a:SetEntityVariable(NS..'.cue',tostring(os.time()+P.cue_delay_seconds));a:SetEntityVariable(NS..'.state','instructions')
 -- Explicit noncombat policy. Zero DB damage/radius is not an AI guarantee.
 -- Preserve targetability and spell eligibility for original Lightning Strike.
 a:SetSpecialAbility(19,1);a:SetSpecialAbility(24,1);a:SetSpecialAbility(25,1);a:WipeHateList()
 a:SetTargetable(true);a:SetTimer(P.timer,P.interval_seconds)
 c:Message(15,'Grolak prepares the golem. Wait for his signal before using the captured lightning.')
 Supplies.recover(c,P.giver)
 return true
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not tagged(n)then return end
 local c=eq.get_entity_list():GetClientByCharID(tonumber(n:GetEntityVariable(NS..'.owner')))
 local born=tonumber(n:GetEntityVariable(NS..'.born'))
 if not enabled()or not c or not c.valid or run(c)~=n:GetEntityVariable(NS..'.run')
  or not yes(c:IsTaskActive(P.task))or not born or os.time()-born>=P.lifetime_seconds then remove(n);return end
 local giver=eq.get_entity_list():GetNPCByID(tonumber(n:GetEntityVariable(NS..'.giver')))
 if not giver or not giver.valid or giver:GetNPCTypeID()~=P.giver then remove(n);return end
 local state=n:GetEntityVariable(NS..'.state')
 if state=='instructions'and(active(c,10)or active(c,11))and nearby(c,n)
  and os.time()>=(tonumber(n:GetEntityVariable(NS..'.cue'))or math.huge)then
  -- The owner witnesses the actual owned scene's signal. This never completes
  -- the later Lightning Strike objective without its successful original cast.
  n:SetEntityVariable(NS..'.state','credit_pending')
  c:Message(15,'Grolak signals: now! Use the captured lightning on the golem.')
  if active(c,10)then c:UpdateTaskActivity(P.task,10,1)end
  if active(c,11)then
   n:SetEntityVariable(NS..'.opened',tostring(os.time()));n:SetEntityVariable(NS..'.state','window')
   Supplies.recover(c,P.giver)
  else eq.debug('Grolak instruction progress unconfirmed; no automatic replay.')end
 elseif state=='window'and os.time()-(tonumber(n:GetEntityVariable(NS..'.opened'))or 0)>P.window_seconds then
  n:SetEntityVariable(NS..'.state','waiting_ready')
  c:Message(15,'The lightning window closed. Tell Grolak you are ready in Goblin to try his signal again.')
 end
end
function M.cast(e)
 local c,t=e.self,e.target
 if tonumber(e.cast_item_id)~=68487 or not active(c,11)or not yes(e.cast_item_charge_consumed)
  or tonumber(e.caster_id)~=c:GetID()or not e.spell or not e.spell.valid or e.spell:ID()~=33705
  or not t or not t.valid or not t:IsNPC()or tonumber(e.target_id)~=t:GetID()then return end
 local n=t:CastToNPC()
 if not tagged(n)or n:GetEntityVariable(NS..'.owner')~=tostring(c:CharacterID())
  or n:GetEntityVariable(NS..'.run')~=run(c)or n:GetEntityVariable(NS..'.state')~='window'
  or os.time()-(tonumber(n:GetEntityVariable(NS..'.opened'))or 0)>P.window_seconds
  or n:GetHP()<=0 then return end
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 if dx*dx+dy*dy+dz*dz>100^2 or not c:CheckLoS(n)then return end
 n:SetEntityVariable(NS..'.state','complete');c:UpdateTaskActivity(P.task,11,1)
 if active(c,12)then remove(n)else eq.debug('Golem action progress unconfirmed; no extra charge deletion or replay.')end
end
function M.clear(c)
 if not c or not c.valid then return end
 for n in eq.get_entity_list():GetNPCList().entries do
  if tagged(n)and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())and n:GetEntityVariable(NS..'.run')==run(c)then remove(n)end
 end
end
return M
