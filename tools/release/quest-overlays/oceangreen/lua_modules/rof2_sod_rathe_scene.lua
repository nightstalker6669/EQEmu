-- One original Murdunk shared mission. Native instance/member/history and
-- scene CAS services are reused. No task acceptance or generic death is a win.
local D=require('rof2_sod_rathe_data')
local M={}
local run
local function binding()
 local z=eq.get_zone()
 return D.ready and z:GetZoneID()==476 and z:GetInstanceVersion()==D.mission.version and
  z:GetInstanceID()>0 and D.mission or nil
end
local function current()
 return run and binding()==run.b and eq.get_zone():GetInstanceID()==run.instance
end
local function member(c)
 return current() and c and c.valid and c:IsTaskActive(run.b.task) and
  c:GetSharedTaskIdentity()==run.id and
  c:GetSharedTaskInstanceID(run.b.task,476,run.b.version)==run.instance
end
local function participant()
 if not current() then return nil end
 for c in eq.get_entity_list():GetClientList().entries do if member(c) then return c end end
end
local function fail(message)
 if not current() or run.failed or run.success then return end
 if eq.get_zone():FailSodScene(run.id) then
  run.failed=true
  eq.zone_emote(15,message)
  for id,a in pairs(run.actors) do
   if a.alive then local n=eq.get_entity_list():GetNPCByID(id)
    if n.valid and n:GetNPCTypeID()==a.npc and n:GetEntityVariable('rathe_run')==run.id then n:Depop(false) end
   end
  end
 end
end
local function restore(c)
 if not member(c) then return end
 -- Actual arrival inside the exact owned instance, not the request dialogue.
 if c:IsTaskActivityActive(run.b.task,0) then c:UpdateTaskActivity(run.b.task,0,1) end
 c:SodRestoreSceneVictory(run.b.task)
end
function M.enter(c)
 local b=binding()
 if not b or not c.valid or not c:IsTaskActive(b.task) or
  c:GetSharedTaskInstanceID(b.task,476,b.version)~=eq.get_zone():GetInstanceID() then return end
 if run and current() then restore(c);return end
 local state=c:SodSceneState(b.task)
 if state==2 then c:SodRestoreSceneVictory(b.task);return end
 if state==1 then eq.get_zone():FailSodScene(c:GetSharedTaskIdentity());return end
 if state~=0 then return end
 run={b=b,id=c:GetSharedTaskIdentity(),instance=eq.get_zone():GetInstanceID(),
  actors={},spawned=0,killed=0,escaped=0,started=false,success=false,failed=false}
 restore(c)
end
local function spawn(c)
 if run.spawned>=run.b.pool then return end
 local ordinal=run.spawned+1
 local draw=c:SodSceneDraw(run.b.task,ordinal,#run.b.troops)
 if draw<0 or not run.b.troops[draw+1] then fail('A reinforcement role could not be verified.');return end
 local npc_id=run.b.troops[draw+1]
 local p=run.b.origins[npc_id]
 if not p then fail('The reinforcement body origin is unbound.');return end
 local mob=eq.spawn2(npc_id,0,0,p[1],p[2],p[3],p[4] or 0)
 if not mob.valid or not mob:IsNPC() then fail('A reinforcement could not enter the mission.');return end
 local n=mob:CastToNPC()
 local ground=n:FindGroundZ(p[1],p[2],p[3]+8-n:GetZ())
 local fixed=n:GetFixedZ(p[1],p[2],p[3])
 if ground~=ground or fixed~=fixed or math.abs(ground-run.b.origin_floor)>2 or math.abs(fixed-p[3])>8 then
  n:Depop(false);fail('The reinforcement entrance floor could not be verified.');return
 end
 n:SetEntityVariable('rathe_run',run.id)
 n:SetEntityVariable('rathe_actor_id',tostring(n:GetID()))
 n:SetRunning(true)
 n:MoveTo(run.b.escape[1],run.b.escape[2],run.b.escape[3],run.b.escape[4] or 0,false)
 -- These are real native moving NPCs. They follow native pathfinding and
 -- stop to fight when intercepted; no timer teleports them to the army.
 run.actors[n:GetID()]={npc=npc_id,ordinal=ordinal,alive=true,credited=false}
 run.spawned=ordinal
end
function M.killed_merit(e)
 if not current() or run.failed or run.success or not member(e.other) then return end
 -- NPC death transfers its entity ID to the corpse before killed_merit.
 -- The spawn-owned native entity variable preserves that original identity;
 -- death_complete independently supplies killed_entity_id for the same NPC.
 local original=tonumber(e.self:GetEntityVariable('rathe_actor_id'))
 local a=original and run.actors[original]
 if a and a.alive and a.npc==e.self:GetNPCTypeID() and e.self:GetEntityVariable('rathe_run')==run.id then
  a.credited=true
 end
end
function M.death(e)
 if not current() or run.failed or run.success or e.self:GetHP()>0 then return end
 local id=tonumber(e.killed_entity_id)
 local a=id and run.actors[id]
 if not a or not a.alive or a.npc~=e.self:GetNPCTypeID() or
  e.self:GetEntityVariable('rathe_run')~=run.id then return end
 a.alive=false
 if not a.credited then fail('A reinforcement died without a verified mission member kill.');return end
 local c=participant()
 if not c then fail('No active mission member could acknowledge the encounter.');return end
 run.killed=run.killed+1
 if run.killed==run.b.required_kills then
  -- Terminal receipt before the final World progress acknowledgement avoids
  -- the already-fixed completion/death callback race. No replayed reward.
  if not c:SodSceneTransition(run.b.task,1,2) then fail('The encounter success could not be acknowledged.');return end
  run.success=true
  eq.zone_emote(15,'Enough reinforcements have been stopped to break Murdunk\'s assault.')
  c:SodRestoreSceneVictory(run.b.task)
 elseif c:IsTaskActivityActive(run.b.task,1) then c:UpdateTaskActivity(run.b.task,1,1) end
end
function M.tick()
 if not current() or run.failed or run.success then return end
 local c=participant()
 if not c then return end
 restore(c)
 if not run.started then
  local players=0
  for p in eq.get_entity_list():GetClientList().entries do if member(p) then players=players+1 end end
  -- Original RoF start condition: three actual participant PCs physically
  -- present. Mercs, remote members and unrelated visitors do not count.
  if players<run.b.start_players then return end
  if not c:SodSceneTransition(run.b.task,0,1) then return end
  run.started=true;run.next_spawn=os.time()+run.b.interval_seconds
 end
 if os.time()>=run.next_spawn and run.spawned<run.b.pool then
  spawn(c);run.next_spawn=os.time()+run.b.interval_seconds
 end
 local alive=0
 for id,a in pairs(run.actors) do
  if a.alive then
   local n=eq.get_entity_list():GetNPCByID(id)
   if not n.valid or n:GetNPCTypeID()~=a.npc or n:GetEntityVariable('rathe_run')~=run.id then
    fail('A tracked reinforcement disappeared before death or escape.');return
   end
   local dx,dy=n:GetX()-run.b.escape[1],n:GetY()-run.b.escape[2]
   if n:GetHP()>0 and not n:IsEngaged() and dx*dx+dy*dy<=run.b.escape_radius*run.b.escape_radius and
    math.abs(n:GetZ()-run.b.escape_floor)<=run.b.escape_vertical_tolerance then
    a.alive=false;run.escaped=run.escaped+1;n:Depop(false)
    eq.zone_emote(15,'A reinforcement has reached Murdunk\'s encampment.')
    if run.escaped>run.b.max_escaped then fail('Too many reinforcements reached Murdunk.');return end
   else alive=alive+1 end
  end
 end
 if run.spawned==run.b.pool and alive==0 and run.killed<run.b.required_kills then
  fail('Too few reinforcements were stopped to break the assault.')
 end
end
function M.register()
 for _,id in ipairs(D.mission.troops or {}) do
  eq.register_npc_event('rof2_sod_rathe',Event.killed_merit,id,M.killed_merit)
  eq.register_npc_event('rof2_sod_rathe',Event.death_complete,id,M.death)
 end
end
function M.unload()
 if current() and run.started and not run.success and not run.failed then fail('The active ambush controller was interrupted.') end
end
return M
