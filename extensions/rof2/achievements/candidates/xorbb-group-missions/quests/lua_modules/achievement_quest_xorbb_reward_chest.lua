-- The three real mission reward chests. Source only, never direct item/reward grants.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local Context=require('achievement_quest_xorbb_mission_context')
local Gems=require('achievement_quest_xorbb_gems_challenges')
local M={}
local routes={
 [753941]={mission='battle',role='battle_reward_chest',task=3040488,spawn_activity=6,final_activity=7},
 [753955]={mission='king',role='king_reward_chest',task=3040489,spawn_activity=4,final_activity=4},
 [753980]={mission='gems',role='gems_reward_chest',task=3040490,spawn_activity=16,final_activity=17}
}
local STATE='xorbb_reward_chest_state'
local LIFETIME='xorbb_reward_chest_native_lifetime'
local function yes(v)return v==true or v==1 end
local function route(n)
 return D.enabled and A.enabled and n and n.valid and n:IsNPC()
  and routes[n:GetNPCTypeID()]or nil
end
-- The instance controller calls this immediately after its own actual chest
-- creation. Never attach by proximity/name, on reload, or to an existing corpse.
function M.prepare_created(c,n)
 local r=route(n)
 if not r or n:GetID()<=0 or n:GetClass()~=62
  or n:GetSpecialAbility(53)~=0 or n:GetEntityVariable(STATE)~=''
  or not c or not c.valid or not c:IsClient()
  or not(yes(c:IsTaskActivityActive(r.task,r.spawn_activity))
   or yes(c:IsTaskActivityActive(r.task,r.final_activity)))then return false end
 if not Context.bind(c,n,r.mission,r.role)then return false end
 -- Native /open supplies the corpse/loot. Trap type0 prevents unrelated LDoN
 -- bonus XP; it does not replace the fixed mission reward selection.
 n:SetLDoNLocked(false)
 n:SetLDoNTrapped(false)
 n:SetLDoNTrapDetected(false)
 n:SetLDoNTrapSpellID(0)
 n:SetLDoNTrapType(0)
 if n:IsLDoNLocked()or n:IsLDoNTrapped()or n:GetLDoNTrapSpellID()~=0
  or n:GetLDoNTrapType()~=0 then return false end
 n:SetEntityVariable(LIFETIME,tostring(n:GetID()))
 n:SetEntityVariable(STATE,'prepared')
 return true
end
-- King terminal challenge production still belongs to its unfinished encounter
-- controller. The caller may supply that controller's fixed pre-terminal hook;
-- missing/uncertain challenge evidence never becomes a clean achievement.
-- Source readiness must remain false until all mission producers are complete.
local function challenges(e,c,r,terminal_hooks)
 if r.mission=='battle'then return {status='status_kills_are_independent'}end
 if r.mission=='gems'then
  local ok,result=pcall(Gems.reward_chest_death,e,c)
  return {status=ok and type(result)=='table'and 'submitted_verdicts'or 'unverified',verdicts=ok and result or nil}
 end
 local hook=terminal_hooks and terminal_hooks.king
 if type(hook)~='function'then return {status='unavailable'}end
 local ok,result=pcall(hook,e,c)
 return {status=ok and type(result)=='table'and 'submitted_verdicts'or 'unverified',verdicts=ok and result or nil}
end
-- EVENT_DEATH_COMPLETE only. HandleLDoNOpen calls Death(client,0,SPELL_UNKNOWN,
-- HandtoHand). Require that native shape, the exact recorded lifetime/corpse,
-- actual client opener, occurrence and final active objective. NPC GetID is0
-- here because Death transferred its old ID to the corpse before this callback.
function M.death(e,terminal_hooks)
 local n=e and e.self
 local r=route(n)
 if not r or n:GetClass()~=62 or n:GetEntityVariable(STATE)~='prepared'
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or not e.corpse or not e.corpse.valid or e.corpse:GetID()~=e.killed_entity_id
  or n:GetEntityVariable(LIFETIME)~=tostring(e.killed_entity_id)
  or e.damage~=0 or e.skill_id~=Skill.HandtoHand or not e.spell or e.spell.valid then return false end
 local opener=eq.get_entity_list():GetMobID(tonumber(e.killer_id)or 0)
 if not opener or not opener.valid or not opener:IsClient()then return false end
 local c=opener:CastToClient()
 local id,instance=Context.actor(c,n,r.mission,r.role)
 if not id or not yes(c:IsTaskActivityActive(r.task,r.final_activity))then return false end
 -- Queue terminal verdicts BEFORE the native last-activity update can retire
 -- shared membership. A verdict is a packet submission, never a durable ACK.
 n:SetEntityVariable(STATE,'pending')
 local result=challenges(e,c,r,terminal_hooks)
 local current,current_instance=Context.member(c,r.mission)
 if current~=id or current_instance~=instance
  or not yes(c:IsTaskActivityActive(r.task,r.final_activity))then return false end
 c:UpdateTaskActivity(r.task,r.final_activity,1)
 n:SetEntityVariable(STATE,'submitted')
 return {task_update_submitted=true,challenge_submission=result}
end
return M
