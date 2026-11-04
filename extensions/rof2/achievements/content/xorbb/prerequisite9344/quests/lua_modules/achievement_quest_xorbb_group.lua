-- May22,2013 report: one member's ladder/DustyHam action updates the group.
-- Only these two evidenced actions are shared. No raid, kill, XP, reward or
-- history fabrication. Source owner retains ordinary native kill/loot credit.
local D=require('achievement_quest_xorbb_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local NS='rof2.xorbb.group.v1'
local task=3040474
local function yes(v)return v==true or v==1 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<math.huge end
local function enabled(c)
 return D.enabled==true and D.ready[task]==true and eq.get_zone_id()==753
  and c and c.valid and c:IsClient()and yes(c:IsTaskActive(task))
end
local function eligible(c,a,x,y,z)
 if not enabled(c)or not yes(c:IsTaskActivityActive(task,a))then return false end
 local dx,dy,dz=c:GetX()-x,c:GetY()-y,c:GetZ()-z
 return dx*dx+dy*dy<=D.group_update_xy^2 and math.abs(dz)<=D.group_update_z
end
local function members(c,a,x,y,z,action)
 local group=c:GetGroup()
 if not group or not group.valid then return end
 for index=0,5 do
  local mob=group:GetMember(index)
  if mob and mob.valid and mob:IsClient()then
   local member=mob:CastToClient()
   -- Resolve the member in this exact zone/instance entity list. Remote group
   -- names, bots, mercenaries and offline members cannot receive the action.
   local local_member=eq.get_entity_list():GetClientByCharID(member:CharacterID())
   if member:CharacterID()~=c:CharacterID()and local_member and local_member.valid
    and local_member:GetID()==member:GetID()and eligible(member,a,x,y,z)then
    action(member)
   end
  end
 end
end
function M.ladder(c,door)
 if not enabled(c)or not door or not door.valid or door:GetDoorDBID()~=36451
  or door:GetDoorID()~=8 or door:GetDoorName()~='OBJ_LADDER'
  or I.count(c,66508)<1 or yes(c:IsTaskActivityActive(task,6))then return end
 members(c,6,door:GetX(),door:GetY(),door:GetZ(),function(member)
  -- Native supply/readback before group credit. Never duplicate an existing
  -- carried ladder or bypass the shared helper's occupied-cursor refusal.
  -- A member unable to receive it can use the ordinary clicked source later.
  if I.count(member,66508)>0 or I.give(member,66508)then
   member:UpdateTaskActivity(task,6,1)
  end
 end)
end
function M.before_dusty_pickup(c,object)
 if not object or not object.valid then return end
 if not eligible(c,12,object:GetX(),object:GetY(),object:GetZ())then return end
 -- A pre-pickup callback alone proves nothing. Preserve a transient witness
 -- only for the validated owned source; actual native stage completion and
 -- an increased carried item count must follow synchronously.
 c:SetEntityVariable(NS..'.dusty_before',tostring(I.count(c,66511)))
 c:SetEntityVariable(NS..'.dusty_time',tostring(os.time()))
 for _,axis in ipairs({'x','y','z'})do
  local value=axis=='x'and object:GetX()or axis=='y'and object:GetY()or object:GetZ()
  c:SetEntityVariable(NS..'.dusty_'..axis,tostring(value))
 end
end
function M.stage(e)
 if tonumber(e.task_id)~=task or tonumber(e.activity_id)~=12 then return end
 local c=e.self
 if not enabled(c)then return end
 local before=tonumber(c:GetEntityVariable(NS..'.dusty_before'))
 local when=tonumber(c:GetEntityVariable(NS..'.dusty_time'))
 local x=tonumber(c:GetEntityVariable(NS..'.dusty_x'))
 local y=tonumber(c:GetEntityVariable(NS..'.dusty_y'))
 local z=tonumber(c:GetEntityVariable(NS..'.dusty_z'))
 -- Clear before sharing: group-generated stage callbacks have no witness and
 -- cannot recursively relay credit through another member or a chain of ranges.
 c:SetEntityVariable(NS..'.dusty_before','')
 c:SetEntityVariable(NS..'.dusty_time','')
 if not before or not when or os.time()<when or os.time()-when>D.group_witness_seconds
  or not(finite(x)and finite(y)and finite(z))
  or yes(c:IsTaskActivityActive(task,12))or I.count(c,66511)<=before then return end
 members(c,12,x,y,z,function(member)
  -- A group task update is not an additional physical pickup. Only the real
  -- looter receives the ground item; no extra inventory is invented here.
  member:UpdateTaskActivity(task,12,1)
 end)
end
return M
