-- Original two-area escort and Bitter Victuals controller. Numeric populations,
-- route checkpoints and bounded tuning are documented local reconstruction.
-- Native task history, not this controller, grants Void/Journeyman access.
local D = require('rof2_sod_kithicor_data')
local M = {}
local run
local encounter_name = 'rof2_sod_kithicor'

local function EL() return eq.get_entity_list() end
local function distance(a, p)
  local x,y,z=a:GetX()-p[1],a:GetY()-p[2],a:GetZ()-p[3]
  return math.sqrt(x*x+y*y+z*z)
end
local function phrase(message, text)
  return (message or ''):lower():find(text,1,true) ~= nil
end
local function phase_binding()
  local z=eq.get_zone()
  if z:GetInstanceID()==0 then return nil end
  return D.phases[z:GetZoneID()..':'..z:GetInstanceVersion()]
end
local function current_instance()
  local z=eq.get_zone()
  return run and run.instance==z:GetInstanceID() and run.binding==phase_binding()
end
local function participant()
  if not current_instance() then return nil end
  for c in EL():GetClientList().entries do
    if c.valid and c:IsTaskActive(run.task) and c:GetSharedTaskIdentity()==run.id and
      c:GetSharedTaskInstanceID(run.task,run.binding.zone,run.binding.version)==run.instance then return c end
  end
end
local function fail(reason)
  if not current_instance() or run.failed then return end
  -- Native CAS is phase-local and accepts only ACTIVE -> failed. A delayed
  -- phase0 death/unload can never fail phase0 success or an active phase1.
  if eq.get_zone():FailSodScene(run.id) then
    run.failed=true
    eq.zone_emote(15,reason or 'The escort has failed.')
  end
end
local function bind_static(binding)
  local found
  for n in EL():GetNPCList().entries do
    if n.valid and n:GetNPCTypeID()==binding.npc and n:GetSpawnPointID()==binding.spawn then
      if found then return nil end
      found=n
    end
  end
  return found
end
local function track_static(binding, role, slot)
  local npc=bind_static(binding)
  if not npc then return false end
  local route=run.binding.actor_paths and run.binding.actor_paths[tostring(binding.spawn)]
  if run.binding.path and (not route or route.npc_type_id~=binding.npc or
    route.spawn2_id~=binding.spawn or #route.points~=#run.binding.path) then return false end
  run.actors[npc:GetID()]={npc=binding.npc,spawn=binding.spawn,role=role,slot=slot,
    point=1,home={npc:GetX(),npc:GetY(),npc:GetZ()},path=route and route.points}
  return npc
end
local function ground(npc,p)
  -- FindGroundZ's third argument is RELATIVE to this actor's current Z.
  local z=npc:FindGroundZ(p[1],p[2],p[3]+8-npc:GetZ())
  if z~=z or z < -100000 or math.abs(z-p[3])>8 then return nil end
  return z
end
local function spawn(binding, role)
  local p=binding.position
  local mob=eq.spawn2(binding.npc,0,0,p[1],p[2],p[3],p[4] or 0)
  if not mob.valid or not mob:IsNPC() then fail('A required encounter actor could not be placed.');return nil end
  -- Native eq.spawn2 returns Lua_Mob; MoveTo belongs to Lua_NPC.
  local npc=mob:CastToNPC()
  local floor=ground(npc,p)
  if not floor then npc:Depop(false);fail('A required actor has no verified ground.');return nil end
  npc:MoveTo(p[1],p[2],floor,p[4] or 0,false)
  run.actors[npc:GetID()]={npc=binding.npc,role=role,point=1,home={p[1],p[2],floor}}
  return npc
end
local function setup(c,b)
  local initial_done=0
  if b.task==920192 and b.phase==0 then
    -- setup is called only by enter from a real player/say quest context.
    -- Read that initiator's native shared journal, never the timer context.
    initial_done=eq.get_task_activity_done_count(b.task,2)
    if initial_done<0 or initial_done>#b.initial_raiders or initial_done%1~=0 then return false end
    -- Do not duplicate a surviving wave after an idle controller reload.
    -- Such an ambiguous population needs reconciliation before bootstrap.
    for n in EL():GetNPCList().entries do
      if n.valid and n:GetNPCTypeID()==456903 and n:GetHP()>0 then return false end
    end
  end
  run={id=c:GetSharedTaskIdentity(),task=b.task,instance=eq.get_zone():GetInstanceID(),
    binding=b,actors={},draw=0,waves={},failed=false,escorting=false,initial_done=initial_done}
  if b.task==920192 then
    local colonel=track_static(b.colonel,'colonel')
    if not colonel then run=nil;return false end
    run.colonel=colonel:GetID()
    for i,binding in ipairs(b.refugees) do
      if not track_static(binding,'refugee',i) then run=nil;return false end
    end
    for i,binding in ipairs(b.guards) do
      if not track_static(binding,'guard',i) then run=nil;return false end
    end
  end
  return true
end
local function living_refugees()
  local living={}
  for id,actor in pairs(run.actors) do
    if actor.role=='refugee' then
      local npc=EL():GetNPCByID(id)
      if npc.valid and npc:GetHP()>0 then living[#living+1]=npc end
    end
  end
  table.sort(living,function(a,b) return a:GetSpawnPointID()<b:GetSpawnPointID() end)
  return living
end
local function wave(index,c)
  local binding=run.binding.waves[index]
  if not binding or run.waves[index] then return end
  run.waves[index]=true
  local living=living_refugees()
  if #living<10 then fail('Too many refugees have fallen.');return end
  eq.zone_emote(15,'A wave of panic rolls through the refugees as an ambush appears!')
  for _,entry in ipairs(binding.actors) do
    local npc=spawn(entry,'ambusher')
    if not npc then return end
    run.draw=run.draw+1
    local pick=c:SodSceneDraw(run.task,run.draw,#living)
    if pick<0 then fail('Unable to verify the escort encounter.');return end
    npc:AddToHateList(living[pick+1],1)
    for id,actor in pairs(run.actors) do
      if actor.role=='guard' then
        local guard=EL():GetNPCByID(id)
        if guard.valid and guard:GetHP()>0 and distance(guard,{npc:GetX(),npc:GetY(),npc:GetZ()})<=120 then
          guard:AddToHateList(npc,1)
        end
      end
    end
  end
end
local function start_escort(c,npc)
  if not current_instance() or run.failed or run.task~=920192 or npc:GetID()~=run.colonel or
    c:GetSharedTaskIdentity()~=run.id then return end
  local goal=run.binding.phase==0 and 4 or 9
  if not c:IsTaskActivityActive(run.task,goal) or not c:SodSceneTransition(run.task,0,1) then return end
  run.escorting=true
  local now=os.time()
  for _,actor in pairs(run.actors) do
    if actor.role=='refugee' or actor.role=='guard' or actor.role=='colonel' then
      -- Retain the crowd's spread instead of the post-2013 single-file change.
      actor.start=now+(actor.role=='colonel' and 5 or 0)
      actor.point=1
      actor.moving=false
    end
  end
  npc:Say('All right everyone, let us move out. Stay together and watch for ambushes!')
end
local function move_actor(npc,actor,now)
  if npc:IsEngaged() then actor.moving=false;return end
  if now<(actor.start or now) then return end
  local path=actor.path
  if not path then fail('The escort actor has no verified route.');return end
  local source=path[actor.point]
  if not source then return end
  -- Absolute per-actor corridor; do not add the initial formation offset again.
  local point=source
  if distance(npc,point)<=10 then
    actor.point=actor.point+1
    actor.moving=false
    source=path[actor.point]
    point=source
  end
  if not point then
    -- Arrival was established at the actual final destination above. Retain
    -- this physical position as the idle guard spot instead of homing to spawn
    -- after native non-grid MoveTo ends. This runs once per arrived actor.
    npc:SaveGuardSpot()
    actor.moving=false
    return
  end
  if not actor.moving then
    local floor=ground(npc,point)
    if not floor then fail('The escort path has no verified ground.');return end
    npc:SetRunning(false)
    -- MoveTo walks through native navigation and GetFixedZ. Never teleport
    -- the Colonel/refugees between checkpoints or borrow a player's Z.
    npc:MoveTo(point[1],point[2],floor,point[4] or 0,true)
    actor.moving=true
  end
end

local spells={
  spider_ruminaran={{13989,24}}, spider_cheliceran={{13989,24}}, spider_castigaran={{13989,24}},
  bitter_assassin={{17623,35},{17622,28}},
  bitter_aithoris={{17623,35},{17622,28},{1086,90}},
}
local function bitter_tick(c)
  local now=os.time()
  for _,binding in ipairs(run.binding.population) do
    local npc=bind_static(binding)
    if npc and npc:GetHP()>0 then
      local actor=run.actors[npc:GetID()]
      if not actor then
        track_static(binding,binding.role)
        actor=run.actors[npc:GetID()]
        actor.casts={}
      end
      local target=npc:GetTarget()
      if target.valid and target:IsClient() and target:CastToClient():GetSharedTaskIdentity()==run.id then
        for i,spell in ipairs(spells[binding.role] or {}) do
          actor.casts[i]=actor.casts[i] or (now+5+i*3)
          if now>=actor.casts[i] then
            -- Native casting owns range, resists, cast time and effects.
            if npc:CastSpell(spell[1],target:GetID()) then actor.casts[i]=now+spell[2] end
            break
          end
        end
      end
    end
  end
  -- Remember the actual goal8 hailer, not whichever shared member happens
  -- to be first in the zone list when World's update arrives.
  if not run.poison_attempted and run.poison_hailer then
    local recipient=EL():GetClientByCharID(run.poison_hailer)
    local selkith=bind_static(run.binding.selkith)
    if recipient.valid and recipient:GetSharedTaskIdentity()==run.id and
      recipient:IsTaskActivityActive(run.task,10) and selkith and
      distance(recipient,{selkith:GetX(),selkith:GetY(),selkith:GetZ()})<=25 then
      run.poison_attempted=true
      recipient:SodKithicorPoison(selkith)
    end
  end
end

function M.enter(c)
  local b=phase_binding()
  if not b or not c:IsTaskActive(b.task) or
    c:GetSharedTaskInstanceID(b.task,b.zone,b.version)~=eq.get_zone():GetInstanceID() then return end
  eq.load_encounter(encounter_name)
  if run and not current_instance() then run=nil end
  local state=c:SodSceneState(b.task)
  if not run and state==1 then
    -- An active controller lost on reload is not silently rebuilt/replayed.
    eq.get_zone():FailSodScene(c:GetSharedTaskIdentity())
    return
  end
  if not run and not setup(c,b) then return end
  if b.task==920193 and state==0 then c:SodSceneTransition(b.task,0,1) end
  c:SodRestoreSceneVictory(b.task)
end

function M.say(e)
  local c=e.other
  local b=phase_binding()
  if not b or not c.valid or not c:IsTaskActive(b.task) or
    c:GetSharedTaskInstanceID(b.task,b.zone,b.version)~=eq.get_zone():GetInstanceID() then return end
  if not run then M.enter(c) end
  if not current_instance() or c:GetSharedTaskIdentity()~=run.id then return end
  if b.task==920192 and e.self:GetID()==run.colonel then
    if (b.phase==0 and phrase(e.message,'go')) or (b.phase==1 and phrase(e.message,'moving')) then
      start_escort(c,e.self)
    elseif b.phase==0 and phrase(e.message,'scout') and c:SodSceneState(b.task)==2 and
      c:IsTaskActivityActive(b.task,6) then
      require('rof2_sod_kithicor').travel(c,e.self,false)
    elseif b.phase==1 and phrase(e.message,'ready') and c:SodSceneState(b.task)==2 and
      c:IsTaskActivityActive(b.task,11) then
      M.return_to_camp(c,e.self)
    elseif phrase(e.message,'hail') then
      e.self:Say(b.phase==0 and 'Tell me [go] to begin the march, or [scout] when we reach Kithicor.'
        or 'Scout the path first. Tell me [moving] when the way is ready; I can send you back when you are [ready].')
    end
  elseif b.task==920193 and e.self:GetNPCTypeID()==b.selkith.npc then
    if phrase(e.message,'hail') and c:IsTaskActivityActive(b.task,8) then
      run.poison_hailer=c:CharacterID()
    end
    if phrase(e.message,'hail') or phrase(e.message,'poison') then
      if c:IsTaskActivityActive(b.task,10) then
        if c:SodKithicorPoison(e.self) then
          run.poison_attempted=true
          e.self:Say('Take these six vials and poison their six victuals chests.')
        end
      else
        e.self:Say('Gather the glands, bring them to me, and I will prepare the poison.')
      end
    end
  end
end

function M.death(e)
  if not current_instance() or run.failed then return end
  local actor=run.actors[e.self:GetID()]
  if not actor or actor.npc~=e.self:GetNPCTypeID() then return end
  if actor.spawn and not actor.role:find('spider_',1,true) then eq.disable_spawn2(actor.spawn) end
  if actor.role=='colonel' or actor.role=='refugee' then
    local count=eq.get_zone():KithicorEscortDeath(run.id,e.self)
    if count<0 then fail('Unable to verify the refugee escort.')
    elseif count>=11 then run.failed=true end
  end
  run.actors[e.self:GetID()]=nil
end

function M.tick()
  if not current_instance() or run.failed then return end
  local c=participant()
  if not c then return end
  c:SodRestoreSceneVictory(run.task)
  if run.task==920193 then bitter_tick(c);return end
  if run.task~=920192 then return end
  if run.binding.phase==0 and not run.initial and c:IsTaskActivityActive(run.task,2) then
    run.initial=true
    -- Already credited kills survive a state0 reload. Spawn only the
    -- remaining original slots; never replay the credited opening kill.
    for index=run.initial_done+1,#run.binding.initial_raiders do
      local npc=spawn(run.binding.initial_raiders[index],'initial_raider')
      if not npc then return end
      npc:AddToHateList(c,1)
    end
  end
  if not run.escorting or c:SodSceneState(run.task)~=1 then return end
  local now=os.time()
  for id,actor in pairs(run.actors) do
    if actor.role=='refugee' or actor.role=='guard' or actor.role=='colonel' then
      local npc=EL():GetNPCByID(id)
      if npc.valid and npc:GetHP()>0 then move_actor(npc,actor,now) end
    end
  end
  local colonel=EL():GetNPCByID(run.colonel)
  if not colonel.valid or colonel:GetHP()<=0 then fail('Colonel Thalan has fallen.');return end
  local actor=run.actors[run.colonel]
  for index,w in ipairs(run.binding.waves) do
    if actor.point>w.checkpoint then wave(index,c) end
  end
  if actor.point>#run.binding.path then
    if #living_refugees()<10 then fail('Too many refugees have fallen.');return end
    for index in ipairs(run.binding.waves) do if not run.waves[index] then return end end
    local arrived=0
    for _,refugee in ipairs(living_refugees()) do
      local progress=run.actors[refugee:GetID()]
      if progress and progress.point>#run.binding.path then arrived=arrived+1 end
    end
    if arrived<10 then return end
    if c:SodSceneTransition(run.task,1,2) then
      run.escorting=false
      c:SodRestoreSceneVictory(run.task)
      colonel:Say(run.binding.phase==0 and 'We have reached Kithicor. Hail me, then we can [scout] ahead.'
        or 'We have reached the Army of Light. Hail me when you are [ready] to report to Barvian.')
    end
  end
end

function M.return_to_camp(c,npc)
  -- Filled by the native, template-bound return helper in the matched batch.
  return require('rof2_sod_kithicor').travel(c,npc,true)
end

function M.register(encounter)
  local seen,say_seen={},{}
  local function register_say(id)
    if not say_seen[id] then
      say_seen[id]=true
      eq.register_npc_event(encounter_name,Event.say,id,M.say)
    end
  end
  for _,b in pairs(D.phases) do
    if b.colonel then
      seen[b.colonel.npc]=true
      register_say(b.colonel.npc)
      for _,a in ipairs(b.refugees) do seen[a.npc]=true end
      for _,a in ipairs(b.guards) do seen[a.npc]=true end
      for _,a in ipairs(b.initial_raiders or {}) do seen[a.npc]=true end
      for _,w in ipairs(b.waves) do for _,a in ipairs(w.actors) do seen[a.npc]=true end end
    end
    if b.selkith then
      register_say(b.selkith.npc)
      for _,a in ipairs(b.population) do seen[a.npc]=true end
    end
  end
  for id in pairs(seen) do eq.register_npc_event(encounter_name,Event.death_complete,id,M.death) end
  eq.set_timer('kithicor_scene',1000,encounter)
end

function M.unload()
  if current_instance() and run.escorting then fail('The active escort controller was interrupted.') end
end
return M
