-- Real evil-eye corpse lenses; normal merit and native pickup own progression.
local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_prison_data')
local M={}
local NS='rof2.xorbb.lenses.v1'
local function yes(v)return v==true or v==1 end
local function relevant(n)
 return D.enabled==true and D.ready[P.task]==true and eq.get_zone_id()==753
  and n and n.valid and P.eyes[n:GetNPCTypeID()]==true
end
function M.merit(e)
 local n,c=e.self,e.other
 if not relevant(n)or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 if yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,8))then
  n:SetEntityVariable(NS..'.eligible','1')
 end
end
function M.death(e)
 local n,corpse=e.self,e.corpse
 if not relevant(n)or n:GetEntityVariable(NS..'.eligible')~='1'
  or not corpse or not corpse.valid or type(e.killed_entity_id)~='number'
  or e.killed_entity_id<=0 or corpse:GetID()~=e.killed_entity_id
  or n:GetEntityVariable(NS..'.supplied')~=''then return end
 if corpse:CountItem(66521)~=0 then n:SetEntityVariable(NS..'.supplied','other_producer');return end
 local count=math.random(P.lens_min,P.lens_max)
 n:SetEntityVariable(NS..'.supplied','pending')
 for _=1,count do corpse:AddItem(66521,1)end
 if corpse:CountItem(66521)==count then n:SetEntityVariable(NS..'.supplied','done')
 else eq.debug('Xorbb lens corpse supply needs review; no automatic retry.')end
end
return M
