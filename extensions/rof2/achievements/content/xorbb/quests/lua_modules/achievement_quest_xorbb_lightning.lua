-- Plans Gone Awry: original Empty Lightning Box capture action only.
-- The later Grolak demonstration requires its separate owned actor/window.
local D=require('achievement_quest_xorbb_mercenary_data')
local P=require('achievement_quest_xorbb_lightning_data')
local I=require('achievement_quest_mesa_inventory')
local M={};local NS='rof2.xorbb.lightning.v1'
local function yes(v)return v==true or v==1 end
function M.cast(e)
 local c,t=e.self,e.target
 if not D.enabled or D.ready[P.task]~=true or eq.get_zone_id()~=753
  or tonumber(e.cast_item_id)~=68486 or not yes(e.cast_item_charge_consumed)
  or not c or not c.valid or not c:IsClient()or tonumber(e.caster_id)~=c:GetID()
  or not yes(c:IsTaskActive(P.task))or not yes(c:IsTaskActivityActive(P.task,2))
  or not e.spell or not e.spell.valid or e.spell:ID()~=33704
  or not t or not t.valid or not t:IsNPC()then return end
 local n=t:CastToNPC()
 if not P.targets[n:GetNPCTypeID()]or tonumber(e.target_id)~=n:GetID()
  or n:GetHP()<=0 then return end
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 if dx*dx+dy*dy+dz*dz>50^2 or not c:CheckLoS(n)then return end
 local run=c:GetBucket('rof2.xorbb.mercenary.run.'..P.task)
 if not tostring(run):match('^[1-9]%d*$')then return end
 local key=NS..'.'..c:CharacterID()
 if n:GetEntityVariable(key)==run or I.count(c,68487)~=0
  or c:GetInventory():GetItem(Slot.Cursor).valid then return end
 -- Native successful cast already consumed the empty box's charge.
 n:SetEntityVariable(key,run)
 local before=I.count(c,68487);local supplied=I.give(c,68487,1);local after=I.count(c,68487)
 if not supplied or after~=before+1 then
  if after==before then n:SetEntityVariable(key,'')
  else eq.debug('Xorbb lightning custody ambiguous; do not replay.')end
  return
 end
 c:UpdateTaskActivity(P.task,2,1)
 -- No extra item deletion or kill/depop/damage of the shared cloudburst source.
end
return M
