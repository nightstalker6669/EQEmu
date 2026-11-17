-- Original public-zone requester gameplay. No instanced copy or private
-- progression counter substitutes for the native shared occurrence.
local D=require('rof2_sod_fieldofscale_data')
local M={}
local encounter='rof2_sod_fieldofscale_public'
local function enabled()
 local z=eq.get_zone()
 local v=tostring(eq.get_rule('SodProgression:EnableFieldOfScaleSlice')):lower()
 return D.ready and (v=='true' or v=='1') and z:GetInstanceID()==0 and
  z:GetInstanceVersion()==0 and (z:GetZoneID()==452 or z:GetZoneID()==453)
end
function M.refresh(c)
 if enabled() and c.valid and c:IsTaskActive(920197) then c:SodRestoreSceneVictory(920197) end
end
function M.enter(c)
 if not enabled() or not c.valid then return end
 eq.load_encounter(encounter)
 -- Projection only: the timer never selects/kills an actor or grants an item.
 -- It allows World's ordinary delivery acknowledgement to reveal the hidden
 -- goal before replaying an already-earned early death receipt.
 eq.set_timer('fos_public_receipts',1000)
 M.refresh(c)
end
function M.killed_merit(e)
 if not enabled() or eq.get_zone():GetZoneID()~=453 or not D.drake or
  not e.other.valid or not e.other:IsTaskActive(920197) then return end
 local actor=(D.drake.actors or {})[e.self:GetNPCTypeID()]
 if not actor or not actor[e.self:GetSpawnPointID()] then return end
 local state=e.other:SodSceneState(920197)
 if state==2 then M.refresh(e.other);return end
 if state~=0 or not D.drake.selected_necromancer then return end
 -- Native attack.cpp emits KILLED_MERIT to actual credited group/raid/self
 -- clients; this is not an arbitrary death, hail or proximity proxy.
 -- The bound selection rule must be source-qualified and consistent for the same credited death;
 -- no first-kill guarantee or guessed random percentage is supplied here.
 if D.drake.selected_necromancer(e.other,e.self)~=true then return end
 if e.other:SodSceneTransition(920197,0,2) then
  e.other:Message(15,'You feel the curse on the skull drain away as the iksar necromancer dies.')
  M.refresh(e.other)
 end
end
function M.register(encounter_object)
 if not D.ready or not D.drake then return end
 require('rof2_sod_fieldofscale_world').register(encounter_object)
 for id in pairs(D.drake.actors or {}) do
  eq.register_npc_event(encounter,Event.killed_merit,id,M.killed_merit)
 end
end
local function group_clients(c,visit)
 local seen={}
 local function one(target)
  if not target or not target.valid then return end
  -- The current zone entity list is authoritative for a locally present client.
  -- Lua clients do not export GetZoneID/GetInstanceID in this server.
  local present=eq.get_entity_list():GetClientByID(target:GetID())
  if present.valid and not seen[present:GetID()] then
   seen[present:GetID()]=true;visit(present)
  end
 end
 one(c)
 local g=c:GetGroup()
 if g.valid then for i=0,5 do
  local mob=g:GetMember(i)
  if mob.valid and mob:IsClient() then one(mob:CastToClient()) end
 end end
end
function M.illusion(e)
 local p=D.parley
 if not enabled() or not p or eq.get_zone():GetZoneID()~=452 or
  e.self:GetNPCTypeID()~=p.trakanon.npc or e.self:GetSpawnPointID()~=p.trakanon.spawn or
  not e.other.valid or not e.other:SodFieldOfScalePublicRun(920210) or
  not (e.other:IsTaskActivityActive(920210,2) or e.other:IsTaskActivityActive(920210,3)) then return end
 local credited=false
 local id=e.other:GetSharedTaskIdentity()
 group_clients(e.other,function(c)
  -- The original NPC applies its existing instant single-target spell to the
  -- actual nearby group. Do not substitute a mask/item grant or force dismount.
  if c:GetHP()>0 and c:GetHorseId()==0 and
   e.self:CalculateDistance(c:GetX(),c:GetY(),c:GetZ())<=p.cast_range and
   e.self:SpellFinished(p.cast_spell,c) and c:FindBuff(p.cast_spell,e.self:GetID()) then
   if c:IsTaskActive(920210) and c:GetSharedTaskIdentity()==id then credited=true end
  end
 end)
 if credited and e.other:IsTaskActivityActive(920210,2) then e.other:UpdateTaskActivity(920210,2,1) end
end
local function disguised(c,npc)
 -- Both original Trakanon skeleton and the independently-earned Ancient Iksar
 -- mask are valid. Read actual current appearance and the NPC's native con;
 -- no specific buff source, item possession, or invented earned flag is needed.
 local race=c:GetRace()
 if race~=128 and race~=161 then return false end
 local con=c:GetFactionLevel(c:CharacterID(),npc:GetNPCTypeID(),race,c:GetClass(),c:GetDeity(),npc:GetPrimaryFaction(),npc)
 return con>=1 and con<=5
end
function M.ganak(e)
 local p=D.parley
 if not enabled() or not p or eq.get_zone():GetZoneID()~=452 or
  e.self:GetNPCTypeID()~=p.ganak.npc or e.self:GetSpawnPointID()~=p.ganak.spawn or
  not e.other.valid or not e.other:SodFieldOfScalePublicRun(920210) or
  not e.other:IsTaskActivityActive(920210,3) or not disguised(e.other,e.self) then return end
 local text=(e.message or ''):lower()
 if text:find('serve',1,true) then
  -- Only the original final dialogue keyword advances this objective. Actual
  -- guard kills remain ordinary native Kill goals; dead guards are not reset.
  e.other:UpdateTaskActivity(920210,3,1)
  group_clients(e.other,function(c) c:BuffFadeByEffect(58) end)
  for n in eq.get_entity_list():GetNPCList().entries do
   if n.valid and n:GetHP()>0 then for _,b in ipairs(p.guards) do
    if n:GetNPCTypeID()==b.npc and n:GetSpawnPointID()==b.spawn then n:AddToHateList(e.other,1);break end
   end end
  end
 elseif text:find('your master',1,true) then e.self:Say('I know whom you [serve].')
 elseif text:find('parley',1,true) then e.self:Say('Return this answer to [your master].')
 elseif text:find('dragons',1,true) then e.self:Say('Speak of this [parley].')
 elseif text:find('hail',1,true) then e.self:Say('Have you news of the [dragons]?') end
end
return M
