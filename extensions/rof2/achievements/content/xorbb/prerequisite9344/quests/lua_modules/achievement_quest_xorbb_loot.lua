-- Scholarly-minotaur note supply uses real native kill merit and its actual corpse.
-- No inventory grant, artificial kill/loot credit or last-hit-only party guess.
local D=require('achievement_quest_xorbb_data')
local M={}
local prefix='xorbb.crater_note.'
local function yes(v)return v==true or v==1 end
local function relevant(n)
 return D.enabled==true and D.ready[3040474]==true and eq.get_zone_id()==753
  and n and n.valid and n:GetNPCTypeID()==753162
end
function M.merit(e)
 local n,c=e.self,e.other
 if not relevant(n)or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 if not yes(c:IsTaskActive(3040474))or not yes(c:IsTaskActivityActive(3040474,3))then return end
 local who=tostring(c:CharacterID())
 if n:GetEntityVariable(prefix..who)=='1'then return end
 local count=tonumber(n:GetEntityVariable(prefix..'count'))or 0
 if count<0 or count>=72 or count~=math.floor(count)then return end
 n:SetEntityVariable(prefix..who,'1')
 n:SetEntityVariable(prefix..'count',tostring(count+1))
end
function M.death(e)
 local n,corpse=e.self,e.corpse
 if not relevant(n)or not corpse or not corpse.valid then return end
 -- Native death-complete binds the created corpse to the original killed ID;
 -- e.self may already have ID0, so never look up a possibly reused NPC ID.
 if type(e.killed_entity_id)~='number'or e.killed_entity_id<=0 or corpse:GetID()~=e.killed_entity_id then return end
 if n:GetEntityVariable(prefix..'supplied')~=''then return end
 local count=tonumber(n:GetEntityVariable(prefix..'count'))or 0
 if count<1 or count>72 or count~=math.floor(count)then return end
 if corpse:CountItem(66504)~=0 then
  n:SetEntityVariable(prefix..'supplied','other_producer')
  eq.debug('Xorbb crater-note corpse already has notes; refusing a second producer.')
  return
 end
 -- Mark before mutation so a repeated callback cannot duplicate a partial add.
 n:SetEntityVariable(prefix..'supplied','pending')
 for _=1,count do corpse:AddItem(66504,1)end
 if corpse:CountItem(66504)~=count then
  eq.debug('Xorbb crater-note corpse supply needs review; no automatic retry.')
  return
 end
 n:SetEntityVariable(prefix..'supplied','done')
end
return M
