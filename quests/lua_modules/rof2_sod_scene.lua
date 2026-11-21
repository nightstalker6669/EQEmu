-- Approved LOCAL Oceangreen reconstruction. Native task history/archive, not
-- this controller, grants progression. Binding module is assembled from the
-- coordinator-owned final IDs/geometry, never guessed at runtime.
local M = {}
local B = require('rof2_sod_scene_bindings')
local Ward = require('rof2_sod_ward_door')
local run
local EL = function() return eq.get_entity_list() end
local function xyz(n) return {n:GetX(), n:GetY(), n:GetZ()} end
local function distance(n,p)
  local dx,dy,dz=n:GetX()-p[1],n:GetY()-p[2],n:GetZ()-p[3]
  return math.sqrt(dx*dx+dy*dy+dz*dz)
end
local function current_instance()
  if not run then return false end
  local z=eq.get_zone()
  return z:GetZoneID()==run.zone and z:GetInstanceID()==run.instance and z:GetInstanceVersion()==run.version
end
local function participant(living)
  if not current_instance()then return end
  for c in EL():GetClientList().entries do
    if c:GetSharedTaskIdentity()==run.id and c:IsTaskActive(run.task) and
      (not living or c:GetHP()>0) then return c end
  end
end
local function credit(activity,amount)
  local c=participant(false)
  if c and c:IsTaskActivityActive(run.task,activity) then c:UpdateTaskActivity(run.task,activity,amount) end
end
local function clean(predicate)
  for id,a in pairs(run.actors) do
    if predicate(a) then
      local npc=EL():GetNPCByID(id)
      if npc.valid then npc:Depop(false) end
      run.actors[id]=nil
    end
  end
end
local function fail(reason)
  if not run or run.failed or run.won then return end
  run.failed=true;run.reason=reason
  local c=participant(false)
  eq.get_zone():FailSodScene(run.id)
  clean(function() return true end)
end
local function draw(limit)
  run.draw=run.draw+1
  local c=participant(false);local value=c and c:SodSceneDraw(run.task,run.draw,limit) or -1
  if value<0 then fail('unreadable frozen server RNG');return 0 end
  return value
end
local function weighted(list)
  local n=draw(100)
  for _,v in ipairs(list) do n=n-v[2];if n<0 then return v[1] end end
  error('invalid approved weight total')
end
local function spawn(role,p,extra)
  if not run or run.failed then return end
  local id=run.binding.roles[role]
  if not id then fail('unbound actor '..role);return end
  local mob=eq.spawn2(id,0,0,p[1],p[2],p[3],p[4] or 0)
  if not mob.valid then fail('native actor spawn failed');return end
  local a=extra or {};a.role=role;a.home={p[1],p[2],p[3]};a.spawned=os.time();a.npc_id=id
  run.actors[mob:GetID()]=a
  if role=='spirit'or a.dialogue then mob:SetInvul(true)end
  if a.attacker and run.task==920178 and not a.pair then mob:CastSpell(2727,mob:GetID()) end
  if (a.attacker and run.task==920179) or role=='spirit' then
    local target,nearest
    if role=='spirit' then target=participant(true)
    else for id in pairs(run.protected)do
      local n=EL():GetNPCByID(id)
      if n.valid and n:GetHP()>0 then local d=distance(mob,xyz(n));if not nearest or d<nearest then target=n;nearest=d end end
    end end
    if target then mob:AddToHateList(target,1)end
  end
  return mob
end
local function wave(i)
  local count,roles
  if run.task==920178 then
    count=weighted({{1,25},{2,50},{3,25}})
    roles=i<=4 and {{'servant',100}} or (i<=8 and {{'servant',50},{'plaguebearer',50}} or {{'servant',25},{'plaguebearer',50},{'scablord',25}})
    if i==12 and draw(100)<25 then
      local p=run.binding.anchors[draw(#run.binding.anchors)+1]
      spawn('Jarlin',p,{attacker=true,pair=true})
      spawn('Zeralin',{p[1]+6,p[2],p[3]},{attacker=true,pair=true});return
    end
  else
    count=weighted({{2,50},{3,50}})
    roles={{'tainted_ghoul',60},{'scablord',15},{'pusflesh_zombie',10},{'stench_flesh_golem',5},{'blood_crusted_gravebone',5},{'ooze_bloated_deathcap',5}}
    if i==run.named_pulse then spawn('Nacakehoth',run.binding.anchors[1],{attacker=true}) end
  end
  local rare=i==9 and draw(100)<10
  local p=run.binding.anchors[draw(#run.binding.anchors)+1]
  for j=1,count do
    local role=weighted(roles)
    if rare and j==1 then role=run.task==920178 and 'bentwing_fly' or 'rare_stench_golem' end
    local x={p[1]+(j==2 and 6 or (j==3 and -6 or 0)),p[2],p[3]}
    spawn(role,x,{attacker=true,supply=draw(3)+1,last_player_hate=0,last_hp=nil,fear_at=0})
  end
end
local function victory()
  local c=participant(false)
  if not c or not c:SodSceneTransition(run.task,1,2) then fail('victory journal not acknowledged');return end
  run.won=true;run.won_at=os.time()
  clean(function(a)return a.attacker or(a.role=='formation_golem'and not a.dialogue)end)
  for i,role in ipairs(run.binding.host_roles)do spawn(role,run.binding.host_positions[i],{dialogue=true})end
  for _,p in ipairs(run.binding.post_formation)do spawn('formation_golem',p,{dialogue=true})end
  run.dialogue_index=0
  credit(2,1) -- final protect tick: Stop1; Dread third, not all three twice.
end
function M.start(c,npc)
  if not B.ready or run then return false end
  local z=eq.get_zone();local b=B.versions[z:GetZoneID()..':'..z:GetInstanceVersion()]
  if not b or z:GetInstanceID()==0 or not c:IsTaskActive(b.task) or
    c:GetSharedTaskInstanceID(b.task,z:GetZoneID(),z:GetInstanceVersion())~=z:GetInstanceID() then return false end
  if b.zone==467 and npc:GetNPCTypeID()~=b.roles.billik then return false end
  local state=c:SodSceneState(b.task)
  if state==1 or state==4 or state==5 then eq.get_zone():FailSodScene(c:GetSharedTaskIdentity());return false end
  if state==2 or state==6 then c:SodRestoreSceneVictory(b.task);return false end
  if state~=0 or not c:SodSceneTransition(b.task,0,1) then return false end
  c:LockSharedTask(true)
  run={task=b.task,id=c:GetSharedTaskIdentity(),instance=z:GetInstanceID(),zone=z:GetZoneID(),version=z:GetInstanceVersion(),binding=b,start=os.time(),draw=0,pulse=0,actors={},civilian_dead={},leader_dead={},poison=0,empty_since=nil,protected={},phases=0,slavers_dead=0,pups_freed=0,pods={A={dead=0,pups={}},B={dead=0,pups={}},C={dead=0,pups={}}}}
  for n in EL():GetNPCList().entries do
    local role=b.protected[n:GetNPCTypeID()]
    if role and n:GetSpawnPointID()~=b.protected_spawn_ids[n:GetNPCTypeID()]then role=nil end
    if role then
      run.protected[n:GetID()]={role=role,home=xyz(n),npc_id=n:GetNPCTypeID(),spawn2=n:GetSpawnPointID()}
      n:ModifyNPCStat('sod_resident_owned','1')
      local profile=b.resident_profiles[n:GetNPCTypeID()]
      local stats={level=profile.level,max_hp=profile.hp,min_hit=profile.raw_min_damage,max_hit=profile.raw_max_damage,attack_delay=20,
        ac=profile.AC,atk=profile.ATK,str=profile.STR,sta=profile.STA,agi=profile.AGI,dex=profile.DEX,int=profile.INT,wis=profile.WIS,cha=profile.CHA,
        mr=profile.resists.MR,fr=profile.resists.FR,cr=profile.resists.CR,pr=profile.resists.PR,dr=profile.resists.DR,cor=profile.resists.corruption,assist=0}
      for key,value in pairs(stats)do n:ModifyNPCStat(key,tostring(value))end
      n:SetHP(n:GetMaxHP()) -- Instance fields only; never cached NPCType/base rows.
    end
    local initial=b.initial_spawn_roles[n:GetSpawnPointID()]
    if initial and b.roles[initial]~=n:GetNPCTypeID()then initial=nil end
    if initial then
      run.actors[n:GetID()]={role=initial,home=xyz(n),npc_id=n:GetNPCTypeID(),boss=initial~='formation_golem',spawn2=n:GetSpawnPointID(),phase=0,nohate_since=os.time(),regen_at=os.time()+6}
      if initial=='Kegrak'then n:ModifyNPCStat('sod_walkspeed','0.7')end
      if initial=='formation_golem'then
        n:ModifyNPCStat('sod_diversion_exp','100');n:SetBodyType(5,true)
        local profile=b.diversion_profile
        n:ModifyNPCStat('level',tostring(profile.level));n:ModifyNPCStat('max_hp',tostring(profile.hp))
        n:ModifyNPCStat('min_hit',tostring(profile.raw_min_damage));n:ModifyNPCStat('max_hit',tostring(profile.raw_max_damage))
        n:ModifyNPCStat('ac',tostring(profile.AC));n:ModifyNPCStat('atk',tostring(profile.ATK));n:ModifyNPCStat('runspeed',tostring(profile.run_speed_native_factor));n:ModifyNPCStat('attack_delay','20')
        n:SetSpecialAbility(24,0);n:SetSpecialAbility(25,0);n:SetSpecialAbility(39,0);n:SetInvul(false);n:SetHP(n:GetMaxHP())
      end
    end
  end
  local protected_count=0;for _ in pairs(run.protected)do protected_count=protected_count+1 end
  local initial_count=0;for _ in pairs(run.actors)do initial_count=initial_count+1 end
  if (b.zone==467 and protected_count~=16)or(b.task==920180 and initial_count~=4)or(b.task==920183 and initial_count~=1)or(b.task==920179 and initial_count~=9)then fail('original registered starting population missing');return false end
  if b.task==920179 then
    run.named_pulse=weighted({{1,25},{6,50},{0,25}})
  end
  return true
end
local function casualties(id)
  local p=run.protected[id];if not p then return end
  local set=p.role=='civilian' and run.civilian_dead or run.leader_dead;set[id]=true
  local count=0;for _ in pairs(set)do count=count+1 end
  if count>=(p.role=='civilian' and 4 or 3)then fail('protected residents lost')end
end
function M.death(e)
  if not current_instance()or run.failed or run.won then return end
  local id=e.killed_entity_id
  if not id or id==0 then return end
  local p=run.protected[id]
  if p and p.npc_id~=e.self:GetNPCTypeID()then return end
  casualties(id)
  local a=run.actors[id]
  if a and a.npc_id~=e.self:GetNPCTypeID()then return end
  local spawn2=(p and p.spawn2)or(a and a.spawn2)
  if spawn2 then local owned=EL():GetSpawnByID(spawn2);if owned.valid then owned:Disable()end end
  if not a then return end
  run.actors[id]=nil
  if a.role=='Nacakehoth' then
    for j=1,4 do spawn('spirit',{e.self:GetX()+j*2,e.self:GetY(),e.self:GetZ()},{attacker=true,expire=os.time()+30})end
  elseif a.role=='Kegrak' then
    for pod,p in pairs(run.binding.pods)do
      for i=1,4 do
        local x=p[1]+(i-1)*6
        -- C4's former column has no room floor; retain every other tuple.
        if run.task==920183 and pod=='C' and i==4 then x=p[1]-6 end
        spawn('slaver',{x,p[2],p[3]},{pod=pod})
      end
      if pod~='C' then for i=1,4 do
        local pup=spawn('pup',{p[1]+(i-1)*3,p[2]+6,p[3]},{pod=pod})
        if pup then table.insert(run.pods[pod].pups,pup:GetID())end
      end end
    end
  elseif a.role=='slaver' then
    run.slavers_dead=run.slavers_dead+1
    local pod=run.pods[a.pod];pod.dead=pod.dead+1
    if pod.dead==4 and a.pod~='C' then
      for _,pup_id in ipairs(pod.pups)do
        local pup=EL():GetNPCByID(pup_id)
        if not pup.valid or pup:GetNPCTypeID()~=run.binding.roles.pup then fail('registered guarded pup missing');return end
      end
      run.pups_freed=run.pups_freed+4
      for _,pup_id in ipairs(pod.pups)do
        local pup=EL():GetNPCByID(pup_id)
        if pup.valid then pup:MoveTo(pup:GetX()+20,pup:GetY()+20,pup:GetZ(),0,false);run.actors[pup_id].expire=os.time()+5 end
      end
      if run.clue then credit(1,4)else run.queued_pups=(run.queued_pups or 0)+4 end
    end
    if run.slavers_dead==12 and run.pups_freed==8 then spawn('Grohlok',run.binding.Grohlok,{boss=true,phase=0,nohate_since=os.time(),regen_at=os.time()+6})end
  elseif a.role=='Grohlok' then
    if a.phase~=2 or a.trance_until then fail('Grohlok died before phased victory');return end
    local c=participant(false)
    if c and c:SodSceneTransition(run.task,1,2)then run.won=true;run.boss_dead=true;credit(2,1)end
  elseif a.role=='Chief' then fail('Chief killed instead of surrendered')
  elseif a.role=='Brankt' then
    local c=participant(false)
    if not c or c:SodSceneState(run.task)~=5 or not run.key_delivered then fail('Brankt killed before Chief key route');return end
    -- Actual credited living killer only; the native spell remains single-target.
    if c:SodSceneTransition(run.task,5,6)then
      credit(run.binding.Brankt_activity,1)
      if c:SodSceneTransition(run.task,6,2)then run.won=true end
    end
  end
end
function M.pair_engaged(n)
  if not current_instance()or run.task~=920178 or run.pair_transformed then return end
  local source=run.actors[n:GetID()];if not source or not source.pair then return end
  run.pair_transformed=true
  for id,a in pairs(run.actors)do if a.pair then
    local actor=EL():GetNPCByID(id)
    if actor.valid then
      actor:SendIllusionPacket({race=a.role=='Jarlin'and 43 or 17,texture=a.role=='Zeralin'and 2 or 0})
      actor:SetBodyType(a.role=='Jarlin'and 21 or 5,true)
    end
  end end
end
local function gate_tick(a,n)
  if a.role~='plaguebearer'and a.role~='Kegrak'then return end
  if not n:IsEngaged()then a.gated=false;return end
  if n:GetHPRatio()<=15 and not a.gated then a.gated=true;n:CastSpell(36,n:GetID())end
end
local function attacker_tick(id,a,n,now)
  gate_tick(a,n)
  if a.last_hp and n:GetHP()<a.last_hp then a.last_player_hate=now end
  a.last_hp=n:GetHP()
  if n:GetHateListClientCount()>0 then a.last_player_hate=now end
  if run.task~=920178 then return end
  if a.pair then
    if n:IsEngaged()then M.pair_engaged(n)end
    return
  end
  if now>=a.fear_at then
    local target,nearest
    for resident,p in pairs(run.protected)do if p.role=='civilian' and not run.civilian_dead[resident]then
      local civilian=EL():GetNPCByID(resident)
      if civilian.valid then local d=distance(n,xyz(civilian));if d<=20 and(not nearest or d<nearest)then target=civilian;nearest=d end end
    end end
    if target then n:CastSpell(17522,target:GetID());a.fear_at=now+12 end
  end
  if a.supply then
    local point=run.binding.supplies[a.supply]
    if now-a.last_player_hate>=6 and distance(n,point)<=15 then
      a.channel=a.channel or now
      if now-a.channel>=20 then
        run.poison=run.poison+1;spawn('Failure',point,{marker=true});n:Depop(false);run.actors[id]=nil
        if run.poison>=4 then fail('four supplies poisoned')end
      end
    else a.channel=nil;if now-a.last_player_hate>=6 then n:CastToNPC():MoveTo(point[1],point[2],point[3],0,false)end end
  end
end
local function boss_tick(a,n,now)
  gate_tick(a,n)
  if a.role=='Chief'or a.role=='Brankt'then
    if n:IsEngaged()then a.combat_at=a.combat_at or now;a.nohate_since=now
    elseif a.combat_at then a.nohate_since=a.nohate_since or now;if now-a.nohate_since>=30 and not a.surrendered then
      n:WipeHateList();n:SetHP(n:GetMaxHP());n:MoveTo(a.home[1],a.home[2],a.home[3],0,false);a.combat_at=nil;a.first_spell=nil;a.second_spell=nil
    end end
    if distance(n,a.home)>80 and not a.surrendered then n:WipeHateList();n:MoveTo(a.home[1],a.home[2],a.home[3],0,false);n:SetHP(n:GetMaxHP());a.combat_at=nil end
    if a.combat_at and not a.surrendered then
      local target=n:GetTarget()
      if target.valid then
        if a.role=='Chief'then
          if now>=(a.first_spell or a.combat_at+10)then n:CastSpell(17529,target:GetID());a.first_spell=now+40 end
          if now>=(a.second_spell or a.combat_at+30)then n:CastSpell(3230,target:GetID());a.second_spell=now+60 end
        elseif now>=(a.first_spell or a.combat_at+15)then n:CastSpell(17530,target:GetID());a.first_spell=now+30 end
      end
    end
  end
  if a.role=='Grohlok' then
    if n:GetHateListClientCount()>0 then a.nohate_since=now
    else
      a.nohate_since=a.nohate_since or now
      if now>=(a.regen_at or 0) then n:SetHP(math.min(n:GetMaxHP(),n:GetHP()+21600));a.regen_at=now+6 end
      if now-a.nohate_since>=30 and (a.phase>0 or n:GetHP()<n:GetMaxHP()) then
        a.phase=0;a.trance_until=nil;n:SetInvul(false);n:SetHP(n:GetMaxHP());n:WipeHateList()
        clean(function(x)return x.trance_add end)
      end
    end
    if a.trance_until then
      if now>=a.trance_until then n:SetHP(n:GetMaxHP());n:SetInvul(false);a.trance_until=nil end
    elseif (a.phase==0 and n:GetHPRatio()<=50)or(a.phase==1 and n:GetHPRatio()<=30)then
      a.phase=a.phase+1;a.trance_until=now+60;n:SetInvul(true)
      spawn(a.phase==1 and 'trance_shaman' or 'trance_mystic',xyz(n),{trance_add=true})
    end
  elseif a.role=='Chief' and not a.surrendered and n:GetHPRatio()<=50 then
    local c=participant(false)
    if c and c:SodSceneState(run.task)==4 and c:SodSceneTransition(run.task,4,5)then
      a.surrendered=true;n:SetInvul(true);n:WipeHateList();credit(2,1)
      -- Key still requires actual surrender dialogue + owned writer receipt.
      n:Say('Enough! Speak with me and take the key.')
    end
  end
end
local function needs_victory_repair(c,task)
  if task==920183 then return c:IsTaskActivityActive(task,1)or c:IsTaskActivityActive(task,2)end
  if task==920180 then return c:IsTaskActivityActive(task,1)or c:IsTaskActivityActive(task,2)or c:IsTaskActivityActive(task,3)end
  return c:IsTaskActivityActive(task,2)
end
function M.tick()
  if run and not current_instance()then run=nil end
  if not run then
    if not B.ready then return end
    local z=eq.get_zone();local b=B.versions[z:GetZoneID()..':'..z:GetInstanceVersion()]
    if b then for c in EL():GetClientList().entries do
      if c:IsTaskActive(b.task)then if needs_victory_repair(c,b.task)then c:SodRestoreSceneVictory(b.task)end;break end
    end end
    return
  end
  if run.failed then return end
  local now=os.time();local c=participant(false)
  if c and run.task==920180 then
    local state=c:SodSceneState(run.task)
    if state==4 or state==5 or state==6 or state==2 then
      if c:IsTaskActivityActive(run.task,1)or
         ((state==5 or state==6 or state==2)and c:IsTaskActivityActive(run.task,2))then
        c:SodRestoreSceneVictory(run.task)
      end
      if not Ward.open_after_native_receipt(c,run.binding.ward_door)then
        fail('approved native ward binding could not open');return
      end
    end
  end
  for id,a in pairs(run.actors)do if a.expire and now>=a.expire then local n=EL():GetNPCByID(id);if n.valid then n:Depop(false)end;run.actors[id]=nil end end
  if not c then
    run.empty_since=run.empty_since or now
    if now-run.empty_since>=120 then fail('empty instance for120seconds')end
    return
  end
  if run.task==920183 and not run.clue and c:IsTaskActivityActive(run.task,1)then
      run.clue=true;if run.queued_pups then credit(1,run.queued_pups);run.queued_pups=0 end
    end
  if run.won and needs_victory_repair(c,run.task)then c:SodRestoreSceneVictory(run.task)end
  if run.won and run.won_at then
    local offsets={0,5,15,25,40,60}
    local lines={'The village still stands.','The enemy is withdrawing.','We will tend to the wounded.','These defenses have held.','Tell Captain Hiran what happened here.','Return when you are ready.'}
    while run.dialogue_index<6 and now-run.won_at>=offsets[run.dialogue_index+1]do
      run.dialogue_index=run.dialogue_index+1
      for id,a in pairs(run.actors)do if a.role==run.binding.host_roles[(run.dialogue_index%2)+1]then
        local n=EL():GetNPCByID(id);if n.valid then n:Say(lines[run.dialogue_index])end
      end end
    end
  end
  if not run.won then
    if participant(true)then run.empty_since=nil else run.empty_since=run.empty_since or now;if now-run.empty_since>=120 then fail('no living participants');return end end
    if run.task==920178 or run.task==920179 then
      local elapsed=now-run.start
      while run.pulse<12 and elapsed>=60+90*run.pulse do run.pulse=run.pulse+1;wave(run.pulse)end
      if run.task==920179 then
        if elapsed>=420 and not run.protect1 then run.protect1=true;credit(2,1)end
        if elapsed>=780 and not run.protect2 then run.protect2=true;credit(2,1)end
      end
      if elapsed>=1140 then victory()end
    end
    if run.task==920179 then
      for id,p in pairs(run.protected)do if p.role=='leader'then
        local guard=EL():GetNPCByID(id)
        if guard.valid and guard:GetHP()>0 then
          if distance(guard,p.home)>20 then guard:WipeHateList();guard:MoveTo(p.home[1],p.home[2],p.home[3],0,false)
          elseif not guard:IsEngaged()then
            for enemy,a in pairs(run.actors)do if a.attacker then
              local n=EL():GetNPCByID(enemy)
              if n.valid then local target=n:GetTarget()
                if target.valid and run.protected[target:GetID()] and run.protected[target:GetID()].role=='civilian'and distance(guard,xyz(target))<=25 then guard:AddToHateList(n,1);break end
              end
            end end
          end
        end
      end end
    end
    for id,a in pairs(run.actors)do
      local n=EL():GetNPCByID(id)
      if n.valid then
        if a.expire and now>=a.expire then n:Depop(false);run.actors[id]=nil
        elseif a.attacker then attacker_tick(id,a,n,now)
        elseif a.boss then boss_tick(a,n,now)end
      end
    end
  end
end
function M.enter(c)
  if run and not current_instance()then run=nil end
  if not B.ready then return end
  local z=eq.get_zone();local b=B.versions[z:GetZoneID()..':'..z:GetInstanceVersion()]
  if not b or z:GetInstanceID()==0 or not c:IsTaskActive(b.task)then return end
  if c:GetSharedTaskInstanceID(b.task,z:GetZoneID(),z:GetInstanceVersion())~=z:GetInstanceID()then return end
  if b.task==920180 then
    local state=c:SodSceneState(b.task)
    if state==4 or state==5 or state==6 or state==2 then
      if not Ward.open_after_native_receipt(c,b.ward_door)then return end
    end
  end
  eq.load_encounter('rof2_sod_oceangreen')
  local state=c:SodSceneState(b.task)
  if not run and(state==1 or state==4 or state==5)then eq.get_zone():FailSodScene(c:GetSharedTaskIdentity());return end
  c:SodRestoreSceneVictory(b.task)
  if b.zone==468 then M.start(c,nil)end
end
function M.register(encounter)
  local seen={}
  for _,b in pairs(B.versions)do
    for _,id in pairs(b.roles)do seen[id]=true end
    for id in pairs(b.protected)do seen[id]=true end
  end
  eq.register_npc_event('rof2_sod_oceangreen',Event.say,B.versions['468:2'].roles.Chief,function(e)
    if run and run.task==920180 and not run.failed and e.other:GetSharedTaskIdentity()==run.id and e.message:lower():find('key',1,true)then
      if e.other:SodChiefKey(e.self)then run.key_delivered=true;e.self:Say('Take the key. Bring the runestones back with it.')end
    end
  end)
  for _,role in ipairs({'Jarlin','Zeralin'})do
    eq.register_npc_event('rof2_sod_oceangreen',Event.combat,B.versions['467:1'].roles[role],function(e)
      if e.joined then M.pair_engaged(e.self)end
    end)
  end
  for id in pairs(seen)do eq.register_npc_event('rof2_sod_oceangreen',Event.death_complete,id,M.death)end
  eq.set_timer('sod_scene',1000,encounter)
end
function M.current()return run end
return M
