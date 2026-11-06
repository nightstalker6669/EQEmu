-- Exact occurrence/actor ownership for the three Xorbb mission controllers.
-- Registration is allowed only immediately after the controller's owned spawn.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local M={}
local function yes(v)return v==true or v==1 end
function M.member(c,mission)
 local d=D.missions[mission]
 if not D.enabled or not A.enabled or not d or not d.ready
  or not c or not c.valid or not c:IsClient()or not yes(c:IsTaskActive(d.task))
  or eq.get_zone_id()~=753 or eq.get_zone_instance_version()~=d.version then return nil end
 local id=c:GetSharedTaskIdentity()
 local instance=eq.get_zone_instance_id()
 if type(id)~='string'or not id:match('^[1-9]%d*$')or not instance or instance<=0
  or c:GetSharedTaskInstanceID(d.task,753,d.version)~=instance then return nil end
 return id,tostring(instance),d
end
function M.bind(c,n,mission,role)
 local id,instance=M.member(c,mission)
 local actors=A.missions[mission]
 if not id or not actors or not actors.actors[role]or not n or not n.valid
  or not n:IsNPC()or n:GetNPCTypeID()~=actors.actors[role]then return false end
 local fields={xorbb_mission_occurrence=id,xorbb_mission_instance=instance,
  xorbb_mission_name=mission,xorbb_mission_role=role}
 -- Reject a foreign/partial ownership tag rather than quietly stealing it.
 local existing=n:GetEntityVariable('xorbb_mission_occurrence')
 if existing~=''then
  for k,v in pairs(fields)do if n:GetEntityVariable(k)~=v then return false end end
  return true
 end
 for k in pairs(fields)do if n:GetEntityVariable(k)~=''then return false end end
 for k,v in pairs(fields)do n:SetEntityVariable(k,v)end
 for k,v in pairs(fields)do if n:GetEntityVariable(k)~=v then return false end end
 return true
end
function M.actor(c,n,mission,role)
 local id,instance,d=M.member(c,mission)
 local actors=A.missions[mission]
 if not id or not actors or not n or not n.valid or not n:IsNPC()
  or n:GetNPCTypeID()~=actors.actors[role]
  or n:GetEntityVariable('xorbb_mission_occurrence')~=id
  or n:GetEntityVariable('xorbb_mission_instance')~=instance
  or n:GetEntityVariable('xorbb_mission_name')~=mission
  or n:GetEntityVariable('xorbb_mission_role')~=role then return nil end
 return id,instance,d
end
return M
