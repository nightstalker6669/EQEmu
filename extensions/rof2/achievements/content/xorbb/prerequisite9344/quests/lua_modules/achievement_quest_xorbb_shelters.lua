-- Native torch DOT/focus and native melee/death, not click-to-destroy credit.
local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_shelters_data')
local M={}
local NS='rof2.xorbb.waterbrigade.v1'
local function yes(v)return v==true or v==1 end
local function enabled()
 return D.enabled==true and D.ready[P.task]==true and eq.get_zone_id()==753
  and P.profile_ready==true and P.geometry_ready==true
end
local function water(n)return n and n.valid and n:GetNPCTypeID()==P.waterbrigade end
local function owned(n,c)
 return water(n)and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())
  and n:GetEntityVariable(NS..'.wave')~=''
end
local function clear_wave(token)
 for n in eq.get_entity_list():GetNPCList().entries do
  if water(n)and n:GetEntityVariable(NS..'.wave')==token then n:Depop(false)end
 end
end
function M.spawn(e)
 local n=e.self
 if not enabled()or not n or not n.valid or n:GetNPCTypeID()~=P.shelter then return end
 -- Formula50 makes33532 permanent, even though its raw duration column is0.
 -- Its native focus297 adds incoming damage only for spell33531 (limit139).
 -- Preserve the actual torch caster, DOT ticks and native kill attribution.
 -- ApplySpellBuff currently rejects raw duration0 before considering permanent
 -- formula50. The normal SpellFinished path honors IsBuffSpell(formula50).
 if not yes(n:FindBuff(33532))then n:SpellFinished(33532,n)end
 if not yes(n:FindBuff(33532))then
  n:SetEntityVariable(NS..'.wave','failed_focus')
  eq.debug('Xorbb shelter native torch vulnerability could not be applied.')
 end
end
function M.damage(e)
 local n,a=e.self,e.other
 if not enabled()or not n or not n.valid or n:GetNPCTypeID()~=P.shelter
  or type(e.damage)~='number'or e.damage<=0 or not a or not a.valid then return end
 if not yes(n:FindBuff(33532))then M.spawn(e)end
 if n:GetEntityVariable(NS..'.wave')~=''then return end
 local c=a
 if not c:IsClient()then c=a:GetOwner()end
 if not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 if not yes(c:IsTaskActive(P.task))or not yes(c:IsTaskActivityActive(P.task,0))then return end
 local token=n:GetID()..':'..os.time()..':'..math.random(1,2147483647)
 n:SetEntityVariable(NS..'.wave',token)
 for _=1,P.spawn_count do
  local m=eq.spawn2(P.waterbrigade,0,0,n:GetX(),n:GetY(),n:GetZ(),n:GetHeading())
  if not m or not m.valid then
   clear_wave(token);n:SetEntityVariable(NS..'.wave','failed_spawn')
   eq.debug('Xorbb waterbrigade creation failed; no automatic wave retry.');return
  end
  local guard=m:CastToNPC()
  if not water(guard)then
   clear_wave(token);n:SetEntityVariable(NS..'.wave','failed_identity');return
  end
  guard:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()))
  guard:SetEntityVariable(NS..'.wave',token)
  guard:SetEntityVariable(NS..'.expires',tostring(os.time()+P.lifetime))
  guard:SetEntityVariable(NS..'.shelter',tostring(n:GetID()))
  guard:SetEntityVariable(NS..'.heal_due',tostring(os.time()+P.heal_interval))
  if not owned(guard,c)then
   guard:Depop(false);clear_wave(token);n:SetEntityVariable(NS..'.wave','failed_owner');return
  end
  guard:SetTimer(P.timer,P.interval)
 end
 for guard in eq.get_entity_list():GetNPCList().entries do
  if water(guard)and guard:GetEntityVariable(NS..'.wave')==token then guard:AddToHateList(c,1)end
 end
 -- Return nil: EVENT_DAMAGE_TAKEN must not replace or multiply native damage.
end
function M.combat(e)
 local n=e.self
 if not enabled()or not water(n)then return end
 if yes(e.joined)then
  n:SetEntityVariable(NS..'.cold_due',tostring(os.time()+P.cold_interval))
  n:SetTimer(P.timer,P.interval)
 elseif n:GetEntityVariable(NS..'.owner')==''then n:StopTimer(P.timer)end
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not water(n)then return end
 local owner=tonumber(n:GetEntityVariable(NS..'.owner'))
 if owner and n:GetEntityVariable(NS..'.wave')~=''then
  local c=eq.get_entity_list():GetClientByCharID(owner)
  local expires=tonumber(n:GetEntityVariable(NS..'.expires'))
  -- A killed shelter or completed seventh shelter is not a free despawn of
  -- surviving attackers. They remain real enemies while the owner is on task.
  if not c or not c.valid or not yes(c:IsTaskActive(P.task))or not expires or os.time()>=expires then
   n:Depop(false);return
  end
 end
 if not enabled()or n:GetHP()<=0 or n:IsCasting()then return end
 local now=os.time()
 -- The brigade also tries to salvage its own burning tent. Bind the original
 -- native entity AND response token, so a recycled ID cannot heal another tent.
 -- Native HealDamage preserves its normal HP cap and update packets. It never
 -- fabricates damage, player contribution, death or task objective credit.
 local shelter_id=tonumber(n:GetEntityVariable(NS..'.shelter'))
 local heal_due=tonumber(n:GetEntityVariable(NS..'.heal_due'))
 if owner and shelter_id and heal_due and now>=heal_due then
  local tent=eq.get_entity_list():GetNPCByID(shelter_id)
  if tent and tent.valid and tent:GetNPCTypeID()==P.shelter and tent:GetHP()>0
   and tent:GetEntityVariable(NS..'.wave')==n:GetEntityVariable(NS..'.wave')then
   local dx,dy,dz=tent:GetX()-n:GetX(),tent:GetY()-n:GetY(),tent:GetZ()-n:GetZ()
   if dx*dx+dy*dy<=P.heal_xy^2 and math.abs(dz)<=P.heal_z then
    tent:HealDamage(P.heal_amount,n)
   end
  end
  n:SetEntityVariable(NS..'.heal_due',tostring(now+P.heal_interval))
 end
 if not n:IsEngaged()then return end
 local due=tonumber(n:GetEntityVariable(NS..'.cold_due'))
 if not due then n:SetEntityVariable(NS..'.cold_due',tostring(now+P.cold_interval));return end
 local target=n:GetHateTop()
 if now>=due and target and target.valid and yes(n:CastSpell(33529,target:GetID()))then
  n:SetEntityVariable(NS..'.cold_due',tostring(now+P.cold_interval))
 end
end
function M.clear(c)
 if not c or not c.valid or eq.get_zone_id()~=753 then return end
 for n in eq.get_entity_list():GetNPCList().entries do if owned(n,c)then n:Depop(false)end end
end
function M.closed(e)
 if tonumber(e.task_id)==P.task then M.clear(e.self)end
end
return M
