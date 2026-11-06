-- Actual native class62 chest preparation/open effects. Inactive source.
-- NPC::Death transfers its ID to the corpse and clears NPC ID before callbacks.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local P=require('achievement_quest_xorbb_trap_data')
local Context=require('achievement_quest_xorbb_mission_context')
local Challenges=require('achievement_quest_xorbb_gems_challenges')
local M={}
local function enabled()return D.enabled and A.enabled and P.enabled and D.missions.gems.ready end
local function point(p)
 if type(p)~='table'then return false end
 for _,k in ipairs({'x','y','z','h'})do
  if type(p[k])~='number'or p[k]~=p[k]or math.abs(p[k])==math.huge then return false end
 end
 return true
end
-- Controller-proven source_slot must match an actual spawn2 ID when present.
-- A dynamic spawn can carry its reserved source slot only through this owned
-- creation path; no search by chest name or proximity is permitted.
function M.prepare(c,n,source_slot)
 local d=P.traps[source_slot]
 if not enabled()or not d or not d.geometry_ready or not n or not n.valid
  or n:GetClass()~=62 or n:GetSpecialAbility(53)~=0 then return false end
 local native=n:GetSpawnPointID()
 if native~=0 and native~=source_slot then return false end
 if not Context.bind(c,n,'gems','trapped_treasure_chest')then return false end
 local prior=n:GetEntityVariable('xorbb_trap_source_slot')
 if prior~=''then return prior==tostring(source_slot)end
 if not Challenges.bind(n,c,'trapped_treasure_chest')then return false end
 n:SetLDoNLocked(false)
 n:SetLDoNTrapType(0)
 n:SetLDoNTrapSpellID(0)
 n:SetLDoNTrapped(true)
 if n:IsLDoNLocked()or n:GetLDoNTrapType()~=0 or n:GetLDoNTrapSpellID()~=0
  or not n:IsLDoNTrapped()then return false end
 n:SetEntityVariable('xorbb_trap_source_slot',tostring(source_slot))
 return true
end
-- Only EVENT_DEATH_COMPLETE from an owned native /open chest reaches effects.
-- The engine cleared IsLDoNTrapped before this callback: role/lifetime ownership
-- is the classifier. A target change, failed click or depop cannot count.
function M.open_death(e,controller)
 if not enabled()or not e or not e.self or not e.self.valid
  or e.self:GetClass()~=62 or not e.corpse or not e.corpse.valid
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or e.corpse:GetID()~=e.killed_entity_id
  or type(controller)~='table'or type(controller.reserve_trap_open)~='function'then return false end
 local slot=tonumber(e.self:GetEntityVariable('xorbb_trap_source_slot'))
 local d=slot and P.traps[slot]
 if not d or not d.geometry_ready then return false end
 local opener=eq.get_entity_list():GetMobID(tonumber(e.killer_id)or 0)
 if not opener or not opener.valid or not opener:IsClient()then return false end
 local c=opener:CastToClient()
 local id,instance=Context.actor(c,e.self,'gems','trapped_treasure_chest')
 if not id or e.self:GetEntityVariable('xorbb_trap_effect_state')~=''then return false end
 if not Challenges.trapped_chest_death(e)then return false end
 -- Required checked controller receipt is an explicit unfinished dependency.
 -- The existing per-entity marker is not claimed to persist across a restart.
 -- No replay if the claim is absent, rejected, uncertain or already consumed.
 local claim=controller.reserve_trap_open(c,id,instance,slot,e.killed_entity_id)
 if type(claim)~='table'or claim.ok~=true or claim.occurrence~=id
  or tostring(claim.instance)~=instance or claim.source_slot~=slot then return false end
 e.self:SetEntityVariable('xorbb_trap_effect_state','pending')
 if d.effect=='displacement'then
  if not point(d.destination)then return false end
  local p=d.destination
  c:MovePCInstance(753,tonumber(instance),p.x,p.y,p.z,p.h)
  e.self:SetEntityVariable('xorbb_trap_effect_state','displacement_submitted')
  -- Submission is not arrival/clearance proof; no item/task reward is granted.
  return true
 elseif d.effect=='contestants'then
  if type(d.contestants)~='table'or #d.contestants~=2 then return false end
  for _,p in ipairs(d.contestants)do if not point(p)then return false end end
  for _,p in ipairs(d.contestants)do
   local mob=eq.spawn2(A.missions.gems.actors.failed_contestant,0,0,p.x,p.y,p.z,p.h)
   if not mob or not mob.valid or not mob:IsNPC()then return false end
   local n=mob:CastToNPC()
   if not Context.bind(c,n,'gems','failed_contestant')or not Challenges.bind(n,c,'failed_contestant')then return false end
   n:SetEntityVariable('xorbb_contestant_source_slot',tostring(p.source_slot))
   n:AddToHateList(c,1)
  end
  e.self:SetEntityVariable('xorbb_trap_effect_state','contestants_spawned')
  return true
 end
 return false
end
return M
