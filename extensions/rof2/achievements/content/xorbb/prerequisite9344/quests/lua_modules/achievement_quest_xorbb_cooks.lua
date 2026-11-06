-- Real cook kill merit unlocks only its paired ham spit for that quest attempt.
local D=require('achievement_quest_xorbb_data')
local spits=require('achievement_quest_xorbb_spit_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local task,key=3040475,'achievement.xorbb.ham_cook.v1'
local function yes(v)return v==true or v==1 end
local function active(c,a)
 return D.enabled==true and D.ready[task]==true and eq.get_zone_id()==753 and c and c.valid
  and yes(c:IsTaskActive(task))and yes(c:IsTaskActivityActive(task,a))
end
local function cook(n)
 return D.enabled==true and D.ready[task]==true and eq.get_zone_id()==753 and n and n.valid
  and n:GetNPCTypeID()==753124
end
function M.accepted(c,id)
 if id==task and active(c,0)then c:SetBucket(key,'')end
end
function M.merit(e)
 local n,c=e.self,e.other
 if not cook(n)or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 if not active(c,3)then return end
 local spawnpoint=n:GetSpawnPointID()
 for _,s in pairs(spits)do
  if s.spawnpoint==spawnpoint then c:SetBucket(key,tostring(spawnpoint));return end
 end
end
function M.door(e)
 local c,o=e.self,e.door
 if not active(c,3)or not o or not o.valid then return 0 end
 local s=spits[o:GetDoorDBID()]
 if not s or o:GetDoorID()~=s.native or o:GetDoorName()~=s.model then return 0 end
 local dx,dy,dz=c:GetX()-o:GetX(),c:GetY()-o:GetY(),c:GetZ()-o:GetZ()
 if not(dx*dx+dy*dy<=D.door_interaction_xy^2 and math.abs(dz)<=D.door_interaction_z)then return 1 end
 if c:GetBucket(key)~=tostring(s.spawnpoint)then
  c:Message(13,'Defeat the cook guarding this spit first.');return 1
 end
 for n in eq.get_entity_list():GetNPCList().entries do
  if n.valid and n:GetNPCTypeID()==753124 and n:GetSpawnPointID()==s.spawnpoint and n:GetHP()>0 then
   c:Message(13,'The cook is still guarding the ham.');return 1
  end
 end
 -- Reuse a carried quest supply on a repeated authentic click; never fabricate
 -- a Loot event or alter an unrelated inventory/bank item.
 if I.count(c,66514)==0 and not I.give(c,66514)then return 1 end
 if I.count(c,66514)>0 then
  c:UpdateTaskActivity(task,3,1)
  if not yes(c:IsTaskActivityActive(task,3))then c:SetBucket(key,'')end
 end
 return 1
end
function M.combat(e)
 if not cook(e.self)then return end
 if yes(e.joined)then e.self:SetTimer(D.cook.timer,D.cook.spell_interval)
 else e.self:StopTimer(D.cook.timer)end
end
function M.timer(e)
 if e.timer~=D.cook.timer or not cook(e.self)then return false end
 local n=e.self
 if not n:IsEngaged()or n:GetHP()<=0 then n:StopTimer(e.timer);return true end
 if n:IsCasting()then return true end
 local target=n:GetHateRandom()
 if target and target.valid then n:CastSpell(33530,target:GetID())end -- Meat Cleaver
 return true
end
return M
