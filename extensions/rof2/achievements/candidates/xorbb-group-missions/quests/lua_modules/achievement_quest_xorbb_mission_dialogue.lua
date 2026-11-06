-- Original action keywords; concise authored responses belong to NPC adapters.
-- A dialogue does not substitute for the scene transition it requests.
local Context=require('achievement_quest_xorbb_mission_context')
local M={}
local routes={
 {mission='battle',role='battle_thistlebrush',activity=0,phrase='ready',action='begin_battle'},
 {mission='battle',role='battle_thistlebrush',activity=6,phrase='hail',action='battle_reward_chest'},
 {mission='king',role='barleyburr',activity=0,phrase='destroy',action='barleyburr_scene'},
 {mission='king',role='lupex',activity=1,phrase='kill',action='lupex_scene'},
 {mission='king',role='king_xorbb',activity=2,phrase='escape',action='begin_king'},
 {mission='gems',role='griznab',activity=0,phrase='hail',action='begin_gems'},
 {mission='gems',role='belmoore',activity=16,phrase='hail',action='gems_reward_chest'}
}
local function phrase(text,word)
 return tostring(text or ''):lower():find('%f[%a]'..word..'%f[%A]')~=nil
end
local function yes(v)return v==true or v==1 end
-- scene_actions is the instance controller's fixed operation table. Operations
-- must prove their actual owned transition/spawn and deduplicate an already
-- submitted native update. Missing/unready operations never advance a task.
function M.say(e,scene_actions)
 if not e or type(scene_actions)~='table'then return false end
 for _,r in ipairs(routes)do
  local id,instance,d=Context.actor(e.other,e.self,r.mission,r.role)
  if id and yes(e.other:IsTaskActivityActive(d.task,r.activity))and phrase(e.message,r.phrase)then
   local operation=scene_actions[r.action]
   if type(operation)~='function'then return false end
   if operation(e,id,instance)~=true then return false end
   -- An authentic scene may depop its speaker. Preserve the prior actor proof,
   -- then revalidate member/occurrence/instance and active stage after effects.
   local current,current_instance=Context.member(e.other,r.mission)
   if current~=id or current_instance~=instance
    or not yes(e.other:IsTaskActivityActive(d.task,r.activity))then return false end
   e.other:UpdateTaskActivity(d.task,r.activity,1)
   -- This is native update submission, not durable world progress or a reward.
   return true
  end
 end
 return false
end
return M
