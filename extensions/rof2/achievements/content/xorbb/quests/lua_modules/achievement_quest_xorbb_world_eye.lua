-- Narrow ordinary-world-eye source eligibility, shared by capture producers.
-- Group-combat admission is a labelled preservation policy, not recovered Live
-- target-ownership logic. Original rare/partisan/pet/dynamic actors are excluded.
local M={lock_key='rof2.xorbb.world_eye.v1.lock'}
M.types={ [753153]=true,[753169]=true,[753174]=true,[753179]=true,[753182]=true,[753190]=true,[753197]=true,[753205]=true,[753209]=true,[753215]=true,[753220]=true,[753222]=true,[753223]=true,[753312]=true }
local function participant(c,m)
 for _=1,4 do
  if not m or not m.valid then return false end
  if m:GetID()==c:GetID()then return true end
  if m:IsClient()then
   local g=c:GetGroup();return g and g.valid and g:IsGroupMember(m)
  end
  if m:GetOwnerID()==0 then return false end
  m=m:GetOwner()
 end
 return false
end
function M.allowed(c,n,max_hp,range,owned_lock)
 if not c or not c.valid or not c:IsClient()or not n or not n.valid
  or not M.types[n:GetNPCTypeID()]or n:IsPet()or n:GetOwnerID()~=0
  or n:GetSpawnPointID()<=0 or n:GetHP()<=0
  or(max_hp and n:GetHPRatio()>max_hp)then return false end
 local lock=n:GetEntityVariable(M.lock_key)
 if lock~=''and lock~=owned_lock then return false end
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 if dx*dx+dy*dy+dz*dz>range^2 or not c:CheckLoS(n)or not c:IsAttackAllowed(n,true)then return false end
 for h in n:GetHateList().entries do
  if not h.valid or not participant(c,h.ent)then return false end
 end
 return true
end
return M
