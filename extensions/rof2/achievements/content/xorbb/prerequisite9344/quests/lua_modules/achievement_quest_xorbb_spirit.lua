-- Lupex is a guide following the player, not a teleport or timed escort credit.
local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_prison_data')
local M={}
local NS='rof2.xorbb.spirit.v1'
local function yes(v)return v==true or v==1 end
local function active(c)
 return eq.get_zone_id()==753 and D.enabled==true and D.ready[P.task]==true
  and P.profile_ready==true and P.geometry_ready==true and c and c.valid
  and yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,5))
end
local function owned(n,c)
 return n and n.valid and n:GetNPCTypeID()==P.spirit
  and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())
end
function M.start(c)
 if not active(c)then return end
 for n in eq.get_entity_list():GetNPCList().entries do
  if owned(n,c)then return end
 end
 -- The owner just earned the normal Sviir kill credit or resumed that exact
 -- saved follow stage. Use the actual player's location, not a guessed route.
 local m=eq.spawn2(P.spirit,0,0,c:GetX(),c:GetY(),c:GetZ(),c:GetHeading())
 if not m or not m.valid then c:Message(13,'The lupine guide could not appear.');return end
 local n=m:CastToNPC()
 if not n or not n.valid or n:GetNPCTypeID()~=P.spirit then return end
 n:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()))
 n:SetEntityVariable(NS..'.expires',tostring(os.time()+P.lifetime))
 if not owned(n,c)then n:Depop(false);return end
 n:SetTargetable(false);n:SetInvul(true)
 n:SetFollowID(c:GetID());n:SetFollowDistance(P.spirit_follow);n:SetFollowCanRun(true)
 n:Say('Follow the tunnels south and east to the prison. I will travel with you.')
 n:SetTimer(P.timer,P.tick)
end
function M.merit(e)
 local n,c=e.self,e.other
 if not n or not n.valid or n:GetNPCTypeID()~=P.sviir
  or not c or not c.valid or not c:IsClient()then return end
 M.start(c:CastToClient())
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not n or not n.valid or n:GetNPCTypeID()~=P.spirit then return end
 local id=tonumber(n:GetEntityVariable(NS..'.owner'))
 if not id then return end
 local c=eq.get_entity_list():GetClientByCharID(id)
 local expires=tonumber(n:GetEntityVariable(NS..'.expires'))
 if not active(c)or not expires or os.time()>=expires then
  n:Depop(false);return
 end
 -- Existing native following/pathing moves this guide; no MovePC, GMMove,
 -- forced owner coordinates, invented task increments or timer completion.
 if n:GetFollowID()~=c:GetID()then n:SetFollowID(c:GetID())end
end
function M.clear(c)
 if not c or not c.valid or eq.get_zone_id()~=753 then return end
 for n in eq.get_entity_list():GetNPCList().entries do if owned(n,c)then n:Depop(false)end end
end
function M.closed(e)
 if tonumber(e.task_id)==P.task then M.clear(e.self)end
end
return M
