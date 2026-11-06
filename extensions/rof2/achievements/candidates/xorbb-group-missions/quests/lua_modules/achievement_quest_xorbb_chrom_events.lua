-- Native event wiring for the actual owned Chrom. Inactive source only.
-- First creation is the scene controller's job, after the six real deliveries.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local Context=require('achievement_quest_xorbb_mission_context')
local Chrom=require('achievement_quest_xorbb_chrom')
local P=require('achievement_quest_xorbb_chrom_positions')
local M={}
local TASK=3040490
local TIMER='xorbb_chrom_owned_bootstrap'
local STATE='xorbb_chrom_bootstrap_state'
local CHARACTER='xorbb_chrom_bootstrap_character'
local function yes(v)return v==true or v==1 end
local function enabled()
 return D.enabled and D.missions.gems.ready and A.enabled and P.enabled
  and P.geometry_ready and type(eq.xorbb_chrom_state)=='function'
end
local function own_context(n)
 local owner=eq.get_owner()
 return n and n.valid and n:IsNPC()and n:GetID()>0
  and owner and owner.valid and owner:IsNPC()and owner:GetID()==n:GetID()
end
-- Call immediately after the controller's OWNED first spawn, not by searching
-- for a nearby/name-matched actor, on a reload, or from generic EVENT_SPAWN.
-- This only schedules a native NPC event: it never initializes waves in the
-- player/chest context, where eq.set_next_hp_event would affect the wrong owner.
function M.prepare_created(c,n,creation_receipt)
 if not enabled()or not c or not c.valid or not c:IsClient()
  or not n or not n.valid or not n:IsNPC()
  or n:GetNPCTypeID()~=A.missions.gems.actors.chrom
  or n:GetHPRatio()~=100 or n:IsEngaged()
  or n:GetEntityVariable(STATE)~=''
  or not yes(c:IsTaskActivityActive(TASK,13))then return false end
 local id,instance=Context.member(c,'gems')
 if not id or type(creation_receipt)~='table'or creation_receipt.ok~=true
  or creation_receipt.status~='committed'or type(creation_receipt.values)~='table'
  or creation_receipt.values.instance~=instance
  or creation_receipt.values.chrom_creation~='bound'
  or creation_receipt.values.chrom_created_entity~=tostring(n:GetID())then return false end
 local initiator=eq.get_initiator()
 if not initiator or not initiator.valid or not initiator:IsClient()
  or initiator:CastToClient():CharacterID()~=c:CharacterID()then return false end
 -- The quest-manager counter API reads the current initiator. Preserve that
 -- binding rather than accidentally inspecting a different member's task.
 for activity=7,12 do
  if eq.get_task_activity_done_count(TASK,activity)~=1 then return false end
 end
 if type(P.bootstrap_delay_ms)~='number'or P.bootstrap_delay_ms<1
  or P.bootstrap_delay_ms>1000 or P.bootstrap_delay_ms%1~=0 then return false end
 if not Context.bind(c,n,'gems','chrom')then return false end
 n:SetEntityVariable(CHARACTER,tostring(c:CharacterID()))
 n:SetEntityVariable(STATE,'pending')
 -- One millisecond is a configurable dispatch choice, not original encounter timing.
 n:SetTimerMS(TIMER,P.bootstrap_delay_ms)
 return true
end
function M.timer(e)
 if not enabled()or not e or e.timer~=TIMER or not own_context(e.self)
  or e.self:GetNPCTypeID()~=A.missions.gems.actors.chrom then return false end
 local n=e.self
 n:StopTimer(TIMER)
 if n:GetEntityVariable(STATE)~='pending'then return false end
 -- An uncertain/failed bootstrap stays held; no timer retry, healing, new
 -- occurrence, fake gem delivery, or automatic initialization on aggro.
 n:SetEntityVariable(STATE,'held')
 local id=n:GetEntityVariable(CHARACTER)
 if not id:match('^[1-9]%d*$')then return false end
 local c=eq.get_entity_list():GetClientByCharID(tonumber(id))
 if not c or not c.valid or not Context.actor(c,n,'gems','chrom')
  or n:GetHPRatio()~=100 or n:IsEngaged()then return false end
 if not Chrom.begin_new(c,n)then return false end
 n:SetEntityVariable(STATE,'ready')
 return true
end
-- A real re-engagement reuses checked wave state. It does not reset completed
-- waves when the boss heals after a wipe. Missing/pending state remains held.
function M.combat(e)
 if not enabled()or not e or not yes(e.joined)or not own_context(e.self)
  or e.self:GetNPCTypeID()~=A.missions.gems.actors.chrom
  or e.self:GetEntityVariable(STATE)~='ready'then return false end
 local target=e.self:GetTarget()
 if not target or not target.valid then return false end
 local owner=target:IsClient()and target or target:GetUltimateOwner()
 if not owner or not owner.valid or not owner:IsClient()then return false end
 local c=owner:CastToClient()
 if not Context.actor(c,e.self,'gems','chrom')then return false end
 return Chrom.resume(c,e.self)
end
function M.hp(e)
 if not enabled()or not e or not own_context(e.self)
  or e.self:GetEntityVariable(STATE)~='ready'then return false end
 return Chrom.hp(e,P.positions)
end
return M
