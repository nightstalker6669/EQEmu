-- Original public-zone events. Native tasks/occurrences own earned progress.
local D=require('rof2_sod_fieldofscale_data')
local M={}
local encounter='rof2_sod_fieldofscale_public'
local lost
local corpses={{npc=452069,spawn=128974},{npc=452078,spawn=129018},{npc=452103,spawn=129100}}
local function enabled()
 local z=eq.get_zone()
 local rule=tostring(eq.get_rule('SodProgression:EnableFieldOfScaleSlice')):lower()
 return D.ready and (rule=='true' or rule=='1') and z:GetZoneID()==452 and z:GetInstanceID()==0 and z:GetInstanceVersion()==0
end
local function EL() return eq.get_entity_list() end
local function spawn(b)
 local p=b.position
 local m=eq.spawn2(b.npc,0,0,p[1],p[2],p[3],p[4])
 if not m.valid or not m:IsNPC() then return nil end
 local n=m:CastToNPC()
 local ground=n:FindGroundZ(p[1],p[2],p[3]+8-n:GetZ())
 local fixed=n:GetFixedZ(p[1],p[2],p[3])
 if ground~=ground or ground < -100000 or math.abs(ground-p[3])>12 or fixed~=fixed or math.abs(fixed-p[3])>12 then n:Depop(false);return nil end
 n:MoveTo(p[1],p[2],fixed,p[4],false)
 n:SaveGuardSpot()
 return n
end
function M.combat(e)
 if not enabled() then return end
 local id=e.self:GetNPCTypeID()
 if id==452019 or id==452239 then
  -- Real combat arms the native HP crossing callback; no timer polls HP.
  if e.joined and e.self:GetHPRatio()>20 then eq.set_next_hp_event(20) end
 elseif id==452920 and not e.joined and e.self:GetEntityVariable('sod_fos_lost_run')~='' then
  e.self:SaveGuardSpot() -- Native stay-put after actual hate-clear, no teleport.
 end
end
function M.hp(e)
 if not enabled() then return end
 local id=e.self:GetNPCTypeID()
 if id~=452019 and id~=452239 then return end
 if e.inc_hp_event==20 then eq.set_next_hp_event(20);return end
 if e.hp_event~=20 then return end
 eq.set_next_inc_hp_event(20)
 local top=e.self:GetHateTop()
 if not top.valid then return end
 if not top:IsClient() then
  if not top:IsPet() then return end
  top=top:GetOwner()
 end
 if not top.valid or not top:IsClient() then return end
 local c=top:CastToClient()
 for _,task in ipairs({920201,920202,920203}) do
  if c:IsTaskActivityActive(task,1) and c:SodFieldOfScalePublicRun(task) then
   -- One roll for this real crossing and this native run, never per member.
   if math.random(100)<=25 then
    e.self:Say('The Sebilisians have taken our people. I will tell you what I know.')
    c:UpdateTaskActivity(task,1,1)
   end
   return
  end
 end
end
local function near(c,p,radius)
 local dx,dy,dz=c:GetX()-p[1],c:GetY()-p[2],c:GetZ()-p[3]
 return dx*dx+dy*dy+dz*dz<=radius*radius
end
function M.proximity(c)
 if not enabled() or not c.valid or c:GetHP()<=0 then return end
 if c:SodFieldOfScalePublicRun(920209) then
  for i,b in ipairs(corpses) do
   if c:IsTaskActivityActive(920209,i-1) then
    for n in EL():GetNPCList().entries do
     if n.valid and n:GetNPCTypeID()==b.npc and n:GetSpawnPointID()==b.spawn and
      n:CalculateDistance(c:GetX(),c:GetY(),c:GetZ())<=35 then
      c:UpdateTaskActivity(920209,i-1,1);break
     end
    end
   end
  end
  if not lost and c:IsTaskActivityActive(920209,3) then
   local expiry=c:SodFieldOfScalePublicExpires(920209)
   if expiry>0 and c:SodSceneTransition(920209,0,1) then
    lost={id=c:GetSharedTaskIdentity(),expires=expiry,actors={},deaths=0}
    for _,b in ipairs(D.world.lost) do
     local n=spawn(b)
     if not n then c:SodSceneTransition(920209,1,3);return end
     n:SetEntityVariable('sod_fos_lost_run',lost.id)
     n:SetEntityVariable('sod_fos_lost_expiry',tostring(expiry))
     lost.actors[n:GetID()]=true
     -- Source: all three stand at third corpse; no forced spawn autoaggro.
    end
   end
  end
 end
 if c:IsTaskActivityActive(920207,8) and near(c,{-425,2858,-146},60) then
  c:UpdateTaskActivity(920207,8,1)
 end
 if c:SodFieldOfScalePublicRun(920201) and c:IsTaskActivityActive(920201,2) and near(c,{-500,-1940,-42},70) then
  c:UpdateTaskActivity(920201,2,1)
 end
 -- Existing emissary's actual island meeting anchor, not a guessed portal.
 if c:SodFieldOfScalePublicRun(920202) and c:IsTaskActivityActive(920202,2) and near(c,{2813,3325,-107.125},70) then
  c:UpdateTaskActivity(920202,2,1)
 end
end
function M.burynai_death(e)
 if not enabled() or e.self:GetNPCTypeID()~=452105 or not e.other.valid or
  not e.other:SodFieldOfScalePublicRun(920202) then return end
 -- The original task forbids killing the Burynai. This real native credited
 -- kill fails the canonical shared task; no local completion flag is used.
 e.other:FailTask(920202)
end
function M.death(e)
 if not enabled() or not lost or not lost.actors[e.self:GetID()] or e.self:GetHP()>0 then return end
 lost.actors[e.self:GetID()]=nil;lost.deaths=lost.deaths+1
 if lost.deaths==3 then for c in EL():GetClientList().entries do
  if c:GetSharedTaskIdentity()==lost.id and c:SodFieldOfScalePublicRun(920209) then c:SodSceneTransition(920209,1,2);break end
 end end
end
function M.tick()
 if not enabled() then return end
 local time=eq.get_zone_time()
 local open=time.zone_hour>=22 or time.zone_hour<2
 local calethos
 for n in EL():GetNPCList().entries do
  if n.valid and n:GetNPCTypeID()==D.world.calethos.npc and n:GetSpawnPointID()==0 and
   n:GetEntityVariable('sod_fos_calethos')=='original-clock' then
   if open then calethos=n else n:Depop(false) end
  end
  if n.valid and n:GetNPCTypeID()==452920 and n:GetEntityVariable('sod_fos_lost_run')~='' then
   local expiry=tonumber(n:GetEntityVariable('sod_fos_lost_expiry'))
   if expiry and os.time()>=expiry+300 and n:GetHP()>0 then n:Depop(false) end
  end
 end
 if open and not calethos then
  local n=spawn(D.world.calethos)
  if n then n:SetEntityVariable('sod_fos_calethos','original-clock') end
 end
 for c in EL():GetClientList().entries do M.proximity(c) end
 -- Release only our local tracking after the documented lifetime; native
 -- task/replay/archive and ordinary corpse decay remain authoritative.
 if lost and os.time()>=lost.expires+300 then lost=nil end
end
function M.register(encounter_object)
 if not enabled() then return end
 for _,id in ipairs({452019,452239}) do
  eq.register_npc_event(encounter,Event.combat,id,M.combat)
  eq.register_npc_event(encounter,Event.hp,id,M.hp)
 end
 eq.register_npc_event(encounter,Event.killed_merit,452105,M.burynai_death)
 eq.register_npc_event(encounter,Event.combat,452920,M.combat)
 eq.register_npc_event(encounter,Event.death_complete,452920,M.death)
 eq.set_timer('fos_world',1000,encounter_object)
 M.tick()
end
return M
