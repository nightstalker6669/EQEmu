-- An Eye for an Eye: three original successful phial actions. Ordinary native
-- kills, gathering, dialogue, transformations and delivery remain separate.
local D=require('achievement_quest_xorbb_mercenary_data')
local P=require('achievement_quest_xorbb_phials_data')
local M={};local NS='rof2.xorbb.phials.v1'
local function yes(v)return v==true or v==1 end
function M.cast(e)
 local c,t=e.self,e.target;local p=P.actions[tonumber(e.cast_item_id)]
 if not p or not D.enabled or D.ready[P.task]~=true or eq.get_zone_id()~=753
  or not c or not c.valid or not c:IsClient()or not yes(c:IsTaskActive(P.task))
  or not yes(c:IsTaskActivityActive(P.task,p.activity))
  or tonumber(e.caster_id)~=c:GetID()or not yes(e.cast_item_charge_consumed)
  or not e.spell or not e.spell.valid or e.spell:ID()~=p.spell
  or not t or not t.valid or not t:IsNPC()then return end
 local n=t:CastToNPC()
 if not p.targets[n:GetNPCTypeID()]or tonumber(e.target_id)~=n:GetID()or n:GetHP()<=0 then return end
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 -- These three original spells all have range160. Native spell validation and
 -- this same-action readback both apply; no invented remote target action.
 if dx*dx+dy*dy+dz*dz>160^2 or not c:CheckLoS(n)then return end
 local run=c:GetBucket('rof2.xorbb.mercenary.run.'..P.task)
 if not tostring(run):match('^[1-9]%d*$')then return end
 local key=NS..'.'..p.activity..'.'..c:CharacterID()
 if n:GetEntityVariable(key)==run then return end
 n:SetEntityVariable(key,run)
 -- Charge was consumed by the successful native cast. Do not remove another
 -- phial, summon a replacement, depop shared targets or award unrelated steps.
 c:UpdateTaskActivity(P.task,p.activity,1)
end
return M
