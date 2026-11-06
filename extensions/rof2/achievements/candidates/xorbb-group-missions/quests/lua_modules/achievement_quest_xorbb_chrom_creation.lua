-- Six real native gem deliveries -> one reserved, actual Chrom creation.
-- Inactive until the fixed checked creation operations exist and are reviewed.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local P=require('achievement_quest_xorbb_chrom_creation_data')
local Context=require('achievement_quest_xorbb_mission_context')
local Events=require('achievement_quest_xorbb_chrom_events')
local M={}
local function yes(v)return v==true or v==1 end
local function ready()
 if not D.enabled or not A.enabled or not D.missions.gems.ready
  or not P.enabled or not P.geometry_ready or not P.creation_binding_ready
  or type(eq.xorbb_gems_state)~='function'then return false end
 for _,k in ipairs({'x','y','z','h'})do
  if type(P[k])~='number'or P[k]~=P[k]or math.abs(P[k])==math.huge then return false end
 end
 return true
end
local function receipt(r,state,instance,entity)
 return type(r)=='table'and r.ok==true and r.status=='committed'
  and type(r.values)=='table'and r.values.instance==instance
  and r.values.chrom_creation==state
  and(not entity or r.values.chrom_created_entity==tostring(entity))
end
-- Wire only to native EVENT_TASK_STAGE_COMPLETE for the six delivery rows.
-- World-acknowledged progress may invoke this for several members; only the
-- committed absent->pending reservation winner performs the native spawn.
function M.stage_complete(e)
 if not ready()or not e or tonumber(e.task_id)~=3040490 then return false end
 local activity=tonumber(e.activity_id)
 if not activity or activity%1~=0 or activity<7 or activity>12 then return false end
 local c=e.self
 local id,instance=Context.member(c,'gems')
 if not id or not yes(c:IsTaskActivityActive(3040490,13))then return false end
 local owner,initiator=eq.get_owner(),eq.get_initiator()
 if not owner or not owner.valid or not owner:IsClient()or owner:GetID()~=c:GetID()
  or not initiator or not initiator.valid or not initiator:IsClient()
  or initiator:CastToClient():CharacterID()~=c:CharacterID()then return false end
 for a=7,12 do
  if eq.get_task_activity_done_count(3040490,a)~=1 then return false end
 end
 local reserved=eq.xorbb_gems_state(c,id,tonumber(instance),'chrom_creation_reserve','')
 if not receipt(reserved,'pending',instance)then return false end
 local mob=eq.spawn2(A.missions.gems.actors.chrom,0,0,P.x,P.y,P.z,P.h)
 if not mob or not mob.valid or not mob:IsNPC()then return false end
 local n=mob:CastToNPC()
 if n:GetID()<=0 or n:GetNPCTypeID()~=A.missions.gems.actors.chrom
  or n:GetSpawnPointID()~=0 or n:GetHPRatio()~=100 or n:IsEngaged()
  or not Context.bind(c,n,'gems','chrom')then return false end
 local bound=eq.xorbb_gems_state(c,id,tonumber(instance),'chrom_creation_bind',tostring(n:GetID()))
 if not receipt(bound,'bound',instance,n:GetID())then return false end
 -- This passes the actual committed creation receipt. The NPC's own timer
 -- performs wave initialization/arming; this player event must not arm HP.
 return Events.prepare_created(c,n,bound)
end
return M
