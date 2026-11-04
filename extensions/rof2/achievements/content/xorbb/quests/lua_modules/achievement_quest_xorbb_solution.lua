-- The Minotaur Smasher: original solution/item/spells, actual augmented corpse.
-- No synthetic kill/Loot/task credit, no replacement NPC, no base-stat changes.
local D=require('achievement_quest_xorbb_mercenary_data')
local P=require('achievement_quest_xorbb_solution_data')
local M={};local NS='rof2.xorbb.solution.v1'
local function yes(v)return v==true or v==1 end
local function enabled()return D.enabled==true and D.ready[P.task]==true and eq.get_zone_id()==753 end
local function active(c,a)return enabled()and c and c.valid and yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,a))end
local function within(c,n)
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 return dx*dx+dy*dy<=P.range_xy^2 and math.abs(dz)<=P.range_z and c:CheckLoS(n)
end
local function profile(n)return n and n.valid and P.targets[n:GetNPCTypeID()]end
function M.cast(e)
 local c,t=e.self,e.target
 -- Successful native completed cast, actual original item in its cast slot.
 -- Animated Solution has unlimited charges: no fabricated consumption check.
 if not enabled()or not c or not c.valid or not c:IsClient()
  or tonumber(e.cast_item_id)~=66525 or tonumber(e.caster_id)~=c:GetID()
  or not e.spell or not e.spell.valid or e.spell:ID()~=23998
  or not t or not t.valid or not t:IsNPC()then return end
 local slot=tonumber(e.cast_item_slot)
 if not slot or c:GetItemIDAt(slot)~=66525 then return end
 local n=t:CastToNPC();local p=profile(n)
 if not p or not active(c,p.activity)or tonumber(e.target_id)~=n:GetID()
  or n:GetHP()<=0 or n:GetHPRatio()>30 or not within(c,n)
  or not c:IsAttackAllowed(n,true)or n:GetEntityVariable(NS..'.item')~=''then return end
 -- One successful augmentation per actual entity. A corpse supplies a single
 -- shared ordinary loot item; group members do not each manufacture a corpse.
 n:SetEntityVariable(NS..'.item',tostring(p.item))
 n:SetEntityVariable(NS..'.spell',tostring(p.spell))
 n:SetEntityVariable(NS..'.activity',tostring(p.activity))
 n:SetEntityVariable(NS..'.source_character',tostring(c:CharacterID()))
 n:SetTimer(P.timer,P.combat_interval_seconds)
 n:AddToHateList(c,1)
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not enabled()or not profile(n)or n:GetHP()<=0 then return end
 local p=profile(n)
 if tonumber(n:GetEntityVariable(NS..'.item'))~=p.item
  or tonumber(n:GetEntityVariable(NS..'.spell'))~=p.spell then return end
 local target=n:GetTarget()
 if n:IsEngaged()and target and target.valid and target:GetHP()>0
  and n:IsAttackAllowed(target,true)and n:CheckLoS(target)then
  n:SpellFinished(p.spell,target)
 end
end
function M.merit(e)
 local n,c=e.self,e.other;local p=profile(n)
 if not p or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 if active(c,p.activity)and tonumber(n:GetEntityVariable(NS..'.item'))==p.item then
  n:SetEntityVariable(NS..'.eligible','1')
 end
end
function M.death(e)
 local n,corpse=e.self,e.corpse;local p=profile(n)
 if not enabled()or not p or n:GetEntityVariable(NS..'.eligible')~='1'
  or tonumber(n:GetEntityVariable(NS..'.item'))~=p.item
  or n:GetEntityVariable(NS..'.supplied')~=''or not corpse or not corpse.valid
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or corpse:GetID()~=e.killed_entity_id then return end
 n:StopTimer(P.timer)
 -- Mark before randomness/mutation so duplicate death callbacks cannot reroll.
 n:SetEntityVariable(NS..'.supplied','rolled')
 if corpse:CountItem(p.item)~=0 then return end
 if math.random()>P.drop_chance then return end
 n:SetEntityVariable(NS..'.supplied','pending')
 corpse:AddItem(p.item,1)
 if corpse:CountItem(p.item)==1 then n:SetEntityVariable(NS..'.supplied','done')
 else eq.debug('Xorbb animated corpse item delta ambiguous; no replay.')end
end
return M
