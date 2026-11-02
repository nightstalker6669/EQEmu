-- Scoped restoration of the original static quest chests. Native death creates
-- the corpse; native looting/delivery owns all task credit and item custody.
local D = require('achievement_quest_buriedsea_data')
local M = {}
local spawns = {
 [118030]={npc=423140,task=3040913,item=84248},
 [118183]={npc=423140,task=3040913,item=84248},
 [118367]={npc=423140,task=3040913,item=84248},
 [118611]={npc=423140,task=3040913,item=84248},
 [118629]={npc=423140,task=3040913,item=84248},
 [118695]={npc=423140,task=3040913,item=84248},
 [118040]={npc=423141,task=3040907,item=52683},
 [118137]={npc=423141,task=3040907,item=52682},
 [118212]={npc=423141,task=3040907,item=52684},
 [118276]={npc=423141,task=3040907,item=52680},
 [118428]={npc=423141,task=3040907,item=52681}
}
local quest_items = {52680,52681,52682,52683,52684,84248}
local function yes(v) return v == true or v == 1 end
local function contract(npc)
 if eq.get_zone_id() ~= 423 then return end
 local s = spawns[npc:GetSpawnPointID()]
 if s and s.npc == npc:GetNPCTypeID() then return s end
end
local function enabled(s)
 local t = s and D.tasks[s.task]
 return D.enabled and t and t.enabled and t.runtime_dependencies_ready
end
local function prepare(npc,s)
 -- Period players used /open rather than melee or spell damage. The native
 -- open path calls Death directly, so these immunities do not block opening.
 npc:SetSpecialAbility(19,1)
 npc:SetSpecialAbility(20,1)
 npc:SetLDoNLocked(false)
 npc:SetLDoNTrapped(false)
 npc:SetLDoNTrapSpellID(0)
 -- The PEQ templates share a mixed six-item loot table. Replace only those
 -- quest items on these exact spawns, preserving unrelated/custom contents.
 for _,id in ipairs(quest_items) do
  local count = npc:CountLoot()
  for _=1,count do
   local before = npc:CountLoot()
   npc:RemoveItem(id)
   if npc:CountLoot() == before then break end
  end
 end
 npc:AddItem(s.item,1)
end
function M.spawn(e)
 local s = contract(e.self)
 if enabled(s) and eq.get_zone_instance_id() == 0 then prepare(e.self,s) end
end
function M.death(e)
 local s = contract(e.self)
 if not s or not D.enabled then return 0 end
 local other = e.other
 local c = other and other.valid and yes(other:IsClient()) and other:CastToClient() or nil
 if not enabled(s) or eq.get_zone_instance_id() ~= 0 or not c or
    not c.valid or not yes(c:IsTaskActive(s.task)) then
  -- EVENT_DEATH runs before corpse creation for both /open and damage.
  -- Vetoing here also prevents off-task attacks from bypassing /open checks.
  e.self:SetHP(e.self:GetMaxHP())
  e.self:WipeHateList()
  if c and c.valid then c:Message(13,'You must have the associated quest to open this chest.') end
  return 1
 end
 -- Also handles a script reload on a chest that spawned before activation.
 prepare(e.self,s)
 return 0
end
return M
