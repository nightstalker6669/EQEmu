-- Original seven-task Rathe progression. The native task selector, committed
-- history and inventory/reward writers own admission, deliveries and credit.
local D=require('rof2_sod_rathe_data')
local M={}
local encounter='rof2_sod_rathe'
local fleeing={}
local function active()
 local rule=tostring(eq.get_rule('SodProgression:EnableRatheSlice')):lower()
 return D.ready and (rule=='true' or rule=='1') and eq.get_zone():GetZoneID()==476
end
local function public()
 return active() and eq.get_zone():GetInstanceID()==0 and eq.get_zone():GetInstanceVersion()==0
end
local function phrase(e,value) return (e.message or ''):lower():find(value,1,true)~=nil end
function M.say(e,role,id)
 if not public() or not e.other.valid or e.self:GetNPCTypeID()~=id then return end
 if role=='Sterik' then
  if phrase(e,'prove') then eq.task_set_selector(D.sets.Sterik)
  elseif phrase(e,'warrior') then e.self:Say('Can you [prove] your strength?')
  elseif phrase(e,'korascian') or phrase(e,'hail') then e.self:Say('I need true [warriors] against the Korascians.') end
 elseif role=='Nedsin' then
  if phrase(e,'help') or phrase(e,'assistance') or phrase(e,'strike') then
   eq.task_set_selector(D.sets.Nedsin)
  elseif phrase(e,'friends') then e.self:Say('They may need your [assistance].')
  elseif phrase(e,'hail') then e.self:Say('Keep your voice down. Will you [help] find my friends, or are you ready to [strike]?') end
 elseif role=='Ylatra' then
  if phrase(e,'directions') then e.other:SodRatheEnterMission(e.self)
  elseif phrase(e,'strike') then eq.task_set_selector(D.sets.YlatraMission)
  elseif phrase(e,'opportunities') or phrase(e,'defeat') then eq.task_set_selector(D.sets.YlatraPersonal)
  elseif phrase(e,'harm') or phrase(e,'supply') then e.self:Say('Their supply lines offer [opportunities].')
  elseif phrase(e,'humiliate') then e.self:Say('Can you [defeat] Murdunk\'s champions?')
  elseif phrase(e,'mission') or phrase(e,'reinforcement') then e.self:Say('This is the time to [strike], if you and your friends are able.')
  elseif phrase(e,'hail') then e.self:Say('You may [harm] their army or [humiliate] its leaders. Once prepared, ask about a [mission].') end
 end
end
local function thrall(n)
 if not public() or not n or not n.valid then return false end
 local spawns=(D.thralls or {})[n:GetNPCTypeID()]
 return spawns and spawns[n:GetSpawnPointID()]==true
end
function M.thrall_say(e)
 if not thrall(e.self) or not e.other.valid or not phrase(e,'hail') or
  e.other:IsInvisible() or not e.other:IsTaskActivityActive(D.tasks.Whom,0) or
  e.self:GetHP()<=0 or e.self:IsEngaged() or e.self:GetEntityVariable('rathe_thrall_resolved')~='' then return end
 local exit=(D.thrall_exits or {})[e.self:GetSpawnPointID()]
 if not exit or not D.thrall_flee_percent then return end
 local occurrence=e.other:SodRatheThrallRun()
 if occurrence=='' then return end
 -- Native RNG draws once for this actual NPC incarnation, never once per
 -- member. The numerical chance is an explicitly approved local policy.
 if Random.Int(1,100)<=D.thrall_flee_percent then
  e.self:SetEntityVariable('rathe_thrall_resolved','flee')
  e.self:Say('You have freed me. Thank you!')
  e.self:SetRunning(true)
  e.self:MoveTo(exit[1],exit[2],exit[3],exit[4] or 0,false)
  fleeing[e.self:GetID()]={npc=e.self:GetNPCTypeID(),spawn=e.self:GetSpawnPointID(),exit=exit,
   character=e.other:CharacterID(),occurrence=occurrence}
  -- Latch only this hailer and this native personal occurrence. The approved
  -- reconstructed escape route must actually finish before rescue credit.
 else
  e.self:SetEntityVariable('rathe_thrall_resolved','hostile')
  e.self:Say('You will not take me!')
  e.self:AddToHateList(e.other,1)
 end
end
function M.thrall_killed_merit(e)
 if not thrall(e.self) or not e.other.valid or
  e.self:GetEntityVariable('rathe_thrall_resolved')~='hostile' or
  not e.other:IsTaskActivityActive(D.tasks.Whom,0) then return end
 -- Native attack.cpp emits this callback to actual kill-credit recipients
 -- before death_complete. A hostile death can credit each eligible member;
 -- a corpse, arbitrary death or uncredited nearby spectator cannot do so.
 e.other:UpdateTaskActivity(D.tasks.Whom,0,1)
end
function M.public_tick()
 if not public() then return end
 for id,b in pairs(fleeing) do
  local n=eq.get_entity_list():GetNPCByID(id)
  if not n.valid or n:GetNPCTypeID()~=b.npc or n:GetSpawnPointID()~=b.spawn then fleeing[id]=nil
  elseif n:CalculateDistance(b.exit[1],b.exit[2],b.exit[3])<=D.flee_arrival_radius then
   local c=eq.get_entity_list():GetClientByCharID(b.character)
   if c.valid and c:IsTaskActivityActive(D.tasks.Whom,0) and c:SodRatheThrallRun()==b.occurrence then
    c:UpdateTaskActivity(D.tasks.Whom,0,1)
   end
   n:Depop(true);fleeing[id]=nil
  end
 end
 local g=D.gateway
 if not g then return end
 for c in eq.get_entity_list():GetClientList().entries do
  if c.valid and c:GetHP()>0 and c:IsTaskActivityActive(D.tasks.Allies,6) and
   math.abs(c:GetX()-g.center[1])<=g.half[1] and math.abs(c:GetY()-g.center[2])<=g.half[2] and
   math.abs(c:GetZ()-g.center[3])<=g.half[3] then c:UpdateTaskActivity(D.tasks.Allies,6,1) end
 end
end
function M.enter(c)
 if not active() or not c.valid then return end
 eq.load_encounter(encounter)
 if not public() then require('rof2_sod_rathe_scene').enter(c) end
end
function M.register()
 for id in pairs(D.thralls or {}) do
  eq.register_npc_event(encounter,Event.say,id,M.thrall_say)
  eq.register_npc_event(encounter,Event.killed_merit,id,M.thrall_killed_merit)
 end
end
return M
