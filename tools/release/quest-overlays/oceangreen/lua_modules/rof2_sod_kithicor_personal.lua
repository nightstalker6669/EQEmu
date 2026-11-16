-- Actual original-world events. These free quest items and encounter actors
-- never grant task completion, reward currency, Void flags or mercenary tiers.
local M={}
local function base()
  local z=eq.get_zone()
  local rule=eq.get_rule('SodProgression:EnableKithicorSlice'):lower()
  return (rule=='true' or rule=='1') and z:GetZoneID()==456 and z:GetInstanceID()==0 and z:GetInstanceVersion()==0
end
local function item(c,id,charges)
  if c:CountItem(id)==0 then c:SummonItem(id,charges or 1) end
end
function M.accepted(e)
  if not base() or not e.other.valid or not e.other:IsTaskActive(e.task_id) then return end
  if e.task_id==920191 and e.self:GetNPCTypeID()==456178 then item(e.other,37641,12)
  elseif e.task_id==920195 and e.self:GetNPCTypeID()==456171 then item(e.other,37637) end
end
function M.laarthik(e)
  if base() and e.self:GetNPCTypeID()==456175 and e.other:IsTaskActivityActive(920191,0) and
    (e.message or ''):lower():find('hail',1,true) then item(e.other,37638) end
end
function M.replacement(e)
  if base() and e.self:GetNPCTypeID()==456178 and e.other:IsTaskActive(920191) and
    (e.message or ''):lower():find('undertaking',1,true) then item(e.other,37641,12) end
end
local ranger_floor={
  {-1181.5,59.25,82.83958},{-1157.5,59.25,84.04881},
  {-1181.5,75.25,83.52081},{-1157.5,75.25,82.35531},
}
local function spawn_grounded(id,p,h)
  local npc=eq.spawn2(id,0,0,p[1],p[2],p[3],h or 0)
  if not npc.valid then return nil end
  local ground=npc:FindGroundZ(p[1],p[2],p[3]+8-npc:GetZ())
  if ground~=ground or ground < -100000 or math.abs(ground-p[3])>8 then npc:Depop(false);return nil end
  npc:MoveTo(p[1],p[2],ground,h or 0,false)
  return npc
end
function M.pyrrdan(e)
  if not base() or e.self:GetNPCTypeID()~=456061 or e.self:GetSpawnPointID()~=128061 or
    not (e.message or ''):lower():find('hail',1,true) or
    not (e.other:IsTaskActivityActive(920191,10) or e.other:IsTaskActivityActive(920191,11)) then return end
  -- The first genuine hail starts the attack before native SpeakWith credit.
  -- A genuine later hail can retry after the prior attackers are gone; never
  -- delete the task or manufacture a replacement hail objective.
  for n in eq.get_entity_list():GetNPCList().entries do
    if n.valid and n:GetNPCTypeID()==456917 and n:GetHP()>0 and
      n:GetEntityVariable('kithicor_pyrrdan')==tostring(e.self:GetID()) then return end
  end
  e.self:Say('The rangers have found me! Defend me!')
  for _,p in ipairs(ranger_floor) do
    local n=spawn_grounded(456917,p,0)
    if n then
      n:SetEntityVariable('kithicor_pyrrdan',tostring(e.self:GetID()))
      n:AddToHateList(e.self,1)
      e.self:AddToHateList(n,1)
    end
  end
end
function M.avatar_death(e)
  if not base() or e.self:GetNPCTypeID()~=456126 or e.self:GetSpawnPointID()~=128224 or e.self:GetHP()>0 then return end
  -- The actual defeated Avatar reveals the real Aridane, who owns the sole
  -- intended amulet corpse route. No NPC-name or player-position substitute.
  local p={e.self:GetX(),e.self:GetY(),e.self:GetZ()}
  local floor=e.self:FindGroundZ(p[1],p[2],8)
  if floor~=floor or floor < -100000 or math.abs(floor-p[3])>15 then return end
  p[3]=floor
  local n=spawn_grounded(456916,p,e.self:GetHeading())
  if n then n:Say('You have uncovered my deception!') end
end
return M
