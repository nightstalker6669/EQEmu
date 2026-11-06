-- Find the Gems... Or Die! occurrence-scoped challenge producers.
-- Inactive until mission readiness and native event adapters are installed.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local M={}
-- Initial clean values require an atomic, checked occurrence initialization.
-- Checked native operations are proposed; cross-zone failure/terminal ordering remains unresolved.
-- Keep terminal awards disabled until that concrete persistence binding exists.
local initialization_ready=false
local function yes(v)return v==true or v==1 end
local function client(c)return c and c.valid and c:IsClient()end
local function enabled()
 return D.enabled==true and D.missions.gems.ready==true and A.enabled==true
end
local function identity(c)
 if not enabled()or not client(c)or not yes(c:IsTaskActive(3040490))then return nil end
 local id=c:GetSharedTaskIdentity()
 if type(id)=='string'and id:match('^[1-9]%d*$')then return id end
end
local persistence_uncertain={}
local function empty_client()return eq.get_entity_list():GetClientByID(0)end
local function checked(c,id,instance,op,field)
 local r=eq.xorbb_gems_state(c or empty_client(),id,instance,op,field or '')
 if type(r)~='table'or r.ok~=true then persistence_uncertain[id]=true;return nil end
 return r
end
local function latch(id,field,c)
 local instance=c and 0 or eq.get_zone_instance_id()
 local r=checked(c,id,instance,'fail',field)
 return r and r.values[field]=='1'or false
end
local function location(c)
 local id=identity(c)
 if not id or eq.get_zone_id()~=753 or eq.get_zone_instance_version()~=3 then return nil end
 local instance=eq.get_zone_instance_id()
 if not instance or instance<=0 or c:GetSharedTaskInstanceID(3040490,753,3)~=instance then return nil end
 return id,instance
end
-- Unfinished admission hook: native TASK_ACCEPTED currently runs BEFORE
-- LoadClientTaskState refreshes the cached shared occurrence. The new identity
-- may be empty/stale here; this callback cannot initialize a proven clean run.
-- Keep initialization_ready=false until authoritative world admission owns the
-- monitor. Offers, entry, relog, invitations or late timers are not substitutes.
function M.accepted(e)
 if not enabled()or e.task_id~=3040490 or not e.self or not e.self.valid
  or e.self:GetNPCTypeID()~=753191 or eq.get_zone_id()~=753
  or eq.get_zone_instance_id()~=0 then return false end
 local id=identity(e.other)
 -- Do not manufacture a clean run from a late event or partially written keys.
 -- The controller's checked initialization must seed explicit '0' values before
 -- monitoring starts; current source deliberately cannot assert that receipt.
 if not id or not initialization_ready then return false end
 local r=checked(e.other,id,0,'initialize')
 return r and r.values.monitored=='1'or false
end
-- Called by the instance controller immediately after a real owned spawn.
-- Never register an arbitrary nearby chest, corpse or name match.
function M.bind(n,c,role)
 local id,instance=location(c)
 local expected=A.missions.gems.actors[role]
 if not id or not expected or not n or not n.valid or not n:IsNPC()
  or n:GetNPCTypeID()~=expected then return false end
 local instance_text=tostring(instance)
 local prior=n:GetEntityVariable('xorbb_gems_occurrence')
 local prior_instance=n:GetEntityVariable('xorbb_gems_instance')
 if(prior~=''and prior~=id)or(prior_instance~=''and prior_instance~=instance_text)then return false end
 local r=checked(c,id,instance,'bind')
 if not r or r.values.instance~=instance_text then return false end
 n:SetEntityVariable('xorbb_gems_occurrence',id)
 n:SetEntityVariable('xorbb_gems_instance',instance_text)
 n:SetEntityVariable('xorbb_gems_role',role)
 return true
end
local function owned_death(e,role)
 if not enabled()or eq.get_zone_id()~=753 or eq.get_zone_instance_version()~=3
  or not e.self or not e.self.valid or not e.self:IsNPC()
  or e.self:GetNPCTypeID()~=A.missions.gems.actors[role]
  or e.self:GetEntityVariable('xorbb_gems_role')~=role then return nil end
 local id=e.self:GetEntityVariable('xorbb_gems_occurrence')
 local instance=tostring(eq.get_zone_instance_id())
 if type(id)~='string'or not id:match('^[1-9]%d*$')or eq.get_zone_instance_id()<=0
  or e.self:GetEntityVariable('xorbb_gems_instance')~=instance
  or not e.corpse or not e.corpse.valid
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or e.corpse:GetID()~=e.killed_entity_id then return nil end
 return id
end
-- Global player native-death adapter. The whole accepted occurrence counts,
-- including a participant dying outside the instance: an explicit conservative
-- emulator edge policy, not a recovered original zone-boundary rule.
function M.player_death(e)
 local id=identity(e.self)
 return id and latch(id,'participant_death',e.self)or false
end
-- Native chest OPEN destroys the real class62 chest and exports deathcomplete.
-- HandleLDoNOpen has already cleared IsTrapped. Use the registered trap role,
-- never that cleared flag or a visual chest-name heuristic. Depop is not death.
function M.trapped_chest_death(e)
 local id=owned_death(e,'trapped_treasure_chest')
 return id and latch(id,'trapped_open')or false
end
function M.contestant_death(e)
 local id=owned_death(e,'failed_contestant')
 if not id then return false end
 local killer=eq.get_entity_list():GetMobID(tonumber(e.killer_id)or 0)
 if not killer or not killer.valid then return latch(id,'contestant_attribution_unknown')end
 local owner=killer:IsClient()and killer or killer:GetUltimateOwner()
 if not client(owner)then
  -- Environmental/NPC attribution is not positive proof of a qualifying player
  -- kill. Suppress this challenge when attribution cannot establish the rule.
  return latch(id,'contestant_attribution_unknown')
 end
 local killer_id=identity(owner:CastToClient())
 if killer_id==id then return latch(id,'contestant_killed')end
 return latch(id,'contestant_attribution_unknown')
end
-- Invoke from an actual owned reward-chest death BEFORE the last task activity
-- update retires shared membership. This submits absolute one-count updates;
-- it neither completes the task nor claims native rewards on the client's behalf.
function M.reward_chest_death(e,c)
 local id=owned_death(e,'gems_reward_chest')
 local current=location(c)
 if not initialization_ready or not id or current~=id or persistence_uncertain[id]then return false end
 local r=checked(c,id,eq.get_zone_instance_id(),'read')
 -- Native receipts do not yet establish cross-zone death/terminal ordering.
 if not r or r.terminal_ready~=true or r.values.monitored~='1'then return false end
 local flags=r.values
 local verdicts={
  {2075378,flags.contestant_killed=='0'and flags.contestant_attribution_unknown=='0'},
  {2075379,flags.participant_death=='0'},
  {2075380,flags.trapped_open=='0'}
 }
 local submitted={}
 for _,v in ipairs(verdicts)do
  if v[2]then
   submitted[v[1]]=yes(c:AdvanceSharedTaskAchievementProgress(v[1],1,v[1],1))
  else submitted[v[1]]=false end
 end
 -- A true submission is not world durability, displayed completion or reward
 -- acceptance. Native absolute progress tolerates re-submission; no fake ACK.
 return submitted
end
return M
