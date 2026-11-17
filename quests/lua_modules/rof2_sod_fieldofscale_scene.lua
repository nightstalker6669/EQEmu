-- Five original GROUP mission controllers. Database/native task history owns
-- completion and rewards. This module is not activated until exact actor,
-- instance, geometry, spell and item bindings are composed by the owners.
local D=require('rof2_sod_fieldofscale_data')
local M={}
local run
local encounter='rof2_sod_fieldofscale'
local function EL() return eq.get_entity_list() end
local function binding()
 local z=eq.get_zone()
 if z:GetInstanceID()==0 then return nil end
 return D.phases[z:GetZoneID()..':'..z:GetInstanceVersion()]
end
local function current()
 return run and binding()==run.b and eq.get_zone():GetInstanceID()==run.instance
end
local function member(c)
 return current() and c and c.valid and c:IsTaskActive(run.b.task) and
  c:GetSharedTaskIdentity()==run.id and
  c:GetSharedTaskInstanceID(run.b.task,run.b.zone,run.b.version)==run.instance
end
local function participant()
 if not current() then return nil end
 for c in EL():GetClientList().entries do if member(c) then return c end end
end
local function fail(message)
 if not current() or run.failed then return end
 if eq.get_zone():FailSodScene(run.id) then
  run.failed=true
  eq.zone_emote(15,message or 'The active mission controller could not be verified.')
 end
end
local function track(n,b,role,index)
 run.actors[n:GetID()]={npc=b.npc,spawn=b.spawn,role=role,index=index,alive=true}
 n:SetEntityVariable('sod_fos_run',run.id)
 return n
end
local function static_actor(b,role,index)
 local found
 for n in EL():GetNPCList().entries do
  if n.valid and n:GetNPCTypeID()==b.npc and n:GetSpawnPointID()==b.spawn then
   if found then return nil end
   found=n
  end
 end
 if not found or found:GetHP()<=0 then return nil end
 return track(found,b,role,index)
end
local function spawn(b,role,index,c)
 if not b or not b.position or not b.npc then return nil end
 local p=b.position
 local mob=eq.spawn2(b.npc,0,0,p[1],p[2],p[3],p[4] or 0)
 if not mob.valid or not mob:IsNPC() then return nil end
 local npc=mob:CastToNPC()
 local ground=npc:FindGroundZ(p[1],p[2],p[3]+8-npc:GetZ())
 if ground~=ground or ground < -100000 or math.abs(ground-p[3])>8 then
  npc:Depop(false);return nil
 end
 local fixed=npc:GetFixedZ(p[1],p[2],p[3])
 if fixed~=fixed or math.abs(fixed-p[3])>8 then npc:Depop(false);return nil end
 npc:MoveTo(p[1],p[2],fixed,p[4] or 0,false)
 track(npc,b,role,index)
 if member(c) then npc:AddToHateList(c,1) end
 return npc
end
local function count_alive(role)
 local n=0
 for _,a in pairs(run.actors) do if a.role==role and a.alive then n=n+1 end end
 return n
end
local function restore(c)
 if not member(c) then return end
 -- This controller exists only after the real entry into the exact owned
 -- instance. Native Touch11 is a door-switch event, not a zone-arrival event.
 -- Submit that actual arrival through ordinary native shared task progress.
 if c:IsTaskActivityActive(run.b.task,0) then c:UpdateTaskActivity(run.b.task,0,1) end
 c:SodRestoreSceneVictory(run.b.task)
end
local function transition(c,expected,next)
 if not member(c) or not c:SodSceneTransition(run.b.task,expected,next) then return false end
 c:SodRestoreSceneVictory(run.b.task)
 return true
end
local function spawn_wave(c,index)
 local waves=run.b.waves
 if not waves or not waves[index] or run.wave==index then return false end
 local target=run.b.first_wave_target_policy and run.b.first_wave_target_policy(nil,c)
 if not member(target) then run.pending_wave=index;return false end
 run.pending_wave=nil
 run.wave=index
 for i,b in ipairs(waves[index]) do
  if not spawn(b,'defender',i,target) then fail('A required hatchery defender could not be placed.');return false end
 end
 return true
end
function M.enter(c)
 local b=binding()
 if not b or not D.ready or not c.valid or not c:IsTaskActive(b.task) or
  c:GetSharedTaskInstanceID(b.task,b.zone,b.version)~=eq.get_zone():GetInstanceID() then return end
 eq.load_encounter(encounter)
 if run and not current() then run=nil end
 if current() then restore(c);return end
 local state=c:SodSceneState(b.task)
 if state==2 then c:SodRestoreSceneVictory(b.task);return end
 if state==1 or state==4 or state==5 or state==6 then
  -- Never replay waves/selected actors from an interrupted active controller.
  eq.get_zone():FailSodScene(c:GetSharedTaskIdentity());return
 end
 if state~=0 then return end
 run={id=c:GetSharedTaskIdentity(),b=b,instance=eq.get_zone():GetInstanceID(),
  actors={},dead={},wave=0,failed=false,yielded=false}
 for role,actors in pairs(b.initial) do
  for i,a in ipairs(actors) do
   if not static_actor(a,role,i) then run=nil;return end
  end
 end
 if not c:SodSceneTransition(b.task,0,1) then run=nil;return end
 restore(c)
end
function M.death(e)
 if not current() or run.failed or e.self:GetHP()>0 then return end
 -- Native NPC death clears self's entity ID after transferring it to the
 -- corpse. This death-complete field carries the saved original actor ID.
 local killed_id=tonumber(e.killed_entity_id)
 if not killed_id or killed_id<=0 then return end
 local a=run.actors[killed_id]
 if not a or not a.alive or a.npc~=e.self:GetNPCTypeID() or
  (a.spawn and a.spawn~=e.self:GetSpawnPointID()) then return end
 if a.spawn then eq.disable_spawn2(a.spawn) end
 a.alive=false
 run.dead[a.role]=(run.dead[a.role] or 0)+1
 local c=participant()
 if not c then fail('The mission has no verified active participant.');return end
 if run.b.key=='Audience' then
  if a.role=='guard' and run.dead.guard==4 then
   if transition(c,1,4) and not spawn(run.b.boss,'warmaster',1,nil) then
    fail('The disguised agent could not be placed.')
   end
  elseif a.role=='warmaster' then transition(c,4,2) end
 elseif run.b.key=='Brood' then
  if a.role=='egg' and run.dead.egg==21 then
   -- Exact initial aggro target comes from the final bound content policy;
   -- no unsourced random target or FD-proof aggro rule is invented here.
   if not run.b.first_wave_target_policy then fail('The hatchery wave target is unbound.');return end
   local target=run.b.first_wave_target_policy(e,c)
   if not member(target) then run.pending_wave=1;return end
   spawn_wave(target,1)
  elseif a.role=='defender' and count_alive('defender')==0 then
   if run.wave==4 then transition(c,1,2)
   else spawn_wave(c,run.wave+1) end
  end
 elseif run.b.key=='Knowledge' then
  if a.role=='beguiler' and not run.yielded and run.dead.beguiler==3 then
   -- The last-beguiler protective health lock is a post-RoF2013change.
   fail('The last beguiler died before revealing the source of the enchantment.')
  elseif a.role=='gordak' then transition(c,5,2) end
 elseif run.b.key=='Phalanx' then
  if a.role=='trooper' and run.dead.trooper==15 then transition(c,1,2) end
 elseif run.b.key=='Shield' and a.role=='guardian' then transition(c,1,2) end
end
function M.tick()
 if not current() or run.failed then return end
 local c=participant()
 if not c then return end
 restore(c)
 if run.b.key=='Brood' then
  if run.pending_wave then spawn_wave(c,run.pending_wave) end
  local target=run.b.first_wave_target_policy(nil,c)
  if member(target) then for id,a in pairs(run.actors) do
   if a.role=='defender' and a.alive then local n=EL():GetNPCByID(id)
    if n.valid and not n:IsEngaged() then n:AddToHateList(target,1) end
   end
  end end
 end
 if run.b.key=='Shield' then
  local object=EL():GetObjectByDBID(run.b.shield_object)
  if object.valid and math.abs(object:GetZ()-run.b.shield_center[3])>0.01 then object:SetZ(run.b.shield_center[3]) end
 end
 if run.b.key=='Audience' and not run.transformed then
  for id,a in pairs(run.actors) do
   if a.role=='warmaster' and a.alive then
    local n=EL():GetNPCByID(id)
    if not run.b.transform_hp then fail('The agent transformation threshold is unbound.');return end
    if n.valid and n:GetHPRatio()<=run.b.transform_hp then
     if not run.b.vitrik_race then fail('The agent transformation is unbound.');return end
     n:SetRace(run.b.vitrik_race);run.transformed=true
     -- Screech's actual spell/timing/HPconditions are bound native NPC AI,
     -- preserving configured period behavior rather than inventing a timer.
    end
   end
  end
 elseif run.b.key=='Shield' and not run.guardian then
  for actor in EL():GetClientList().entries do
   if member(actor) then
    M.shield(actor,actor)
    local eye=actor:SodFieldOfScaleEye()
    if eye.valid then M.shield(actor,eye) end
   end
  end
 elseif run.b.key=='Knowledge' and run.dead.beguiler==2 and not run.yielded then
  for id,a in pairs(run.actors) do
   if a.role=='beguiler' and a.alive then
    local n=EL():GetNPCByID(id)
    if n.valid and n:GetHP()>0 and n:GetHPRatio()<=30 and transition(c,4,5) then
     run.yielded=true
     n:Say('WAIT! Please do not kill me! Gordak brought the new magics!')
     n:SetInvul(true)
     n:SetSpecialAbility(SpecialAbility.immune_aggro,1)
     n:WipeHateList()
     if not spawn(run.b.gordak,'gordak',1,nil) then fail('Coercer Gordak could not be placed.') end
    end
   end
  end
 end
end
function M.crystal_click(c)
 if not current() or run.failed or run.b.key~='Knowledge' or not member(c) then return end
 -- One-use native provider exists only in the actual carried item17345
 -- ITEM_CLICK opcode frame; no fake spell/effect/cast-on event is substituted.
 local target=c:SodFieldOfScaleCrystalTarget()
 if not target.valid then return end
 local a=run.actors[target:GetID()]
 if not a or a.role~='scholar' or not a.alive then return end
 if target:GetHPRatio()>30 then
  target:Emote('mind is unaffected.');return
 end
 -- Selection may not be inferred as one fixed spawn or an arbitrary per-click
 -- chance. The bound source-qualified policy must decide; nil stays held.
 if not run.b.scholar_success then return end
 local success=run.b.scholar_success(c,target,a,run)
 if success==nil then return end
 if not success then
  target:Emote('shakes its head wildly. Its mind is too weak to handle the crystal.')
  return
 end
 if transition(c,1,4) then
  target:Emote('enters a trance-like state, muttering about beguilers and unknown magics.')
  if not run.b.beguilers or #run.b.beguilers~=3 then fail('The beguiler scene is unbound.');return end
  for i,b in ipairs(run.b.beguilers) do
   if not spawn(b,'beguiler',i,nil) then fail('A required beguiler could not be placed.');return end
  end
 end
end
function M.shield(c,source)
 if not current() or run.failed or run.b.key~='Shield' or not member(c) or run.guardian then return false end
 -- Source is supplied by the bound actual door/object/proximity event adapter.
 -- Until the original physical interaction is resolved, no synthetic click.
 if not run.b.verify_shield_source or not run.b.verify_shield_source(c,source) then return false end
 -- Period2009proximity/Eye reports: approaching too early alerts living
 -- protectors; the final guardian does NOT spawn before all four are dead.
 if count_alive('protector')>0 then
  for id,a in pairs(run.actors) do
   if a.role=='protector' and a.alive then
    local n=EL():GetNPCByID(id)
    if n.valid then
     n:SetSpecialAbility(SpecialAbility.immune_feign_death,1)
     for actor in EL():GetClientList().entries do
      if member(actor) and not n:CheckAggro(actor) then n:AddToHateList(actor,1) end
     end
    end
   end
  end
  return false
 end
 local guardian=spawn(run.b.guardian,'guardian',1,c)
 if not guardian then fail('Shield Guardian Neiska could not be placed.');return false end
 run.guardian=guardian:GetID()
 return true
end
function M.spell_finished(e)
 if not current() or run.failed or run.b.key~='Knowledge' or not e.target or not e.target.valid then return end
 local a=run.actors[e.self:GetID()]
 if a and a.role=='gordak' and a.alive and e.spell and e.spell.valid and e.spell:GetID()==13350 and
  e.target:FindBuff(13350,e.self:GetID()) then e.target:SetBuffDuration(13350,5) end
end
function M.register(encounter_object)
 local seen={}
 for _,b in pairs(D.phases) do
  for role,actors in pairs(b.initial) do
   for _,a in ipairs(actors) do seen[a.npc]=true end
  end
  for _,a in ipairs(b.beguilers or {}) do seen[a.npc]=true end
  if b.boss then seen[b.boss.npc]=true end
  if b.gordak then seen[b.gordak.npc]=true end
  if b.guardian then seen[b.guardian.npc]=true end
  for _,wave in ipairs(b.waves or {}) do for _,a in ipairs(wave) do seen[a.npc]=true end end
 end
 for id in pairs(seen) do eq.register_npc_event(encounter,Event.death_complete,id,M.death) end
 eq.register_npc_event(encounter,Event.cast,452910,M.spell_finished)
 eq.set_timer('fos_scene',500,encounter_object)
end
function M.unload()
 if current() then
  local c=participant()
  if c then
   local state=c:SodSceneState(run.b.task)
   if state==1 or state==4 or state==5 or state==6 then fail('The active mission controller was interrupted.') end
  end
 end
end
return M
