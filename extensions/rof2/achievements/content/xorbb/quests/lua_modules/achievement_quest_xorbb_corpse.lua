-- Quest-specific additions to an actual native corpse. No direct inventory,
-- kill/task credit, player ownership changes, or ordinary loot replacement.
local D=require('achievement_quest_xorbb_mercenary_data')
local P=require('achievement_quest_xorbb_corpse_data')
local M={};local NS='rof2.xorbb.corpse.v1.'
local function yes(v)return v==true or v==1 end
local function enabled()return D.enabled==true and P.profiles_ready==true and eq.get_zone_id()==753 end
local function source(n)
 return n and n.valid and n:IsNPC()and not n:IsPet()and n:GetOwnerID()==0 and n:GetSpawnPointID()>0
end
local function eligible(c,r)
 return D.ready[r.task]==true and yes(c:IsTaskActive(r.task))and yes(c:IsTaskActivityActive(r.task,r.activity))
end
function M.merit(e)
 local n,c=e.self,e.other
 if not enabled()or not source(n)or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 -- Native killed-merit dispatch supplies actual exp-eligible participants.
 -- Multiple group members can establish eligibility but cannot multiply rolls.
 for _,r in ipairs(P.routes)do
  if r.npcs[n:GetNPCTypeID()]and eligible(c,r)then
   n:SetEntityVariable(NS..r.key..'.eligible','1')
  end
 end
end
function M.death(e)
 local n,corpse=e.self,e.corpse
 if not enabled()or not source(n)or not corpse or not corpse.valid
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or corpse:GetID()~=e.killed_entity_id then return end
 for _,r in ipairs(P.routes)do
  local key=NS..r.key
  if D.ready[r.task]==true and r.npcs[n:GetNPCTypeID()]
   and n:GetEntityVariable(key..'.eligible')=='1'
   and n:GetEntityVariable(key..'.rolled')==''then
   -- Mark before random/add: a repeated callback or ambiguous mutation never
   -- rerolls. Existing ordinary loot of the same item remains unchanged.
   n:SetEntityVariable(key..'.rolled','1')
   local chance=tonumber(r.chance)
   if chance and chance==chance and chance>=0 and chance<=1
    and corpse:CountItem(r.item)==0 and chance>0 and math.random()<=chance then
    n:SetEntityVariable(key..'.state','pending')
    local was_empty=corpse:IsEmpty()
    local decay_before=corpse:GetDecayTime()
    corpse:AddItem(r.item,1)
    if corpse:CountItem(r.item)==1 then
     if was_empty then
      corpse:ResetDecayTimer()
      eq.debug('Xorbb filled quest corpse lifetime item='..r.item..' corpse='..corpse:GetID()..' before_ms='..decay_before..' after_ms='..corpse:GetDecayTime())
     end
     n:SetEntityVariable(key..'.state','done')
    else eq.debug('Xorbb corpse addition ambiguous; route quarantined without replay.')end
   end
  end
 end
end
return M
