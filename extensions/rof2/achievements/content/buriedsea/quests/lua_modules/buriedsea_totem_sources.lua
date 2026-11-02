-- Repair specific native corpse sources without altering shared loot tables.
local D=require('achievement_quest_buriedsea_data')
local R=require('buriedsea_totem_rules')
local M={}
local function remove_piece(n)
 local limit=n:CountLoot()
 for _=1,limit do
  local before=n:CountLoot();n:RemoveItem(R.fire_piece)
  if n:CountLoot()==before then break end
 end
end
function M.spawn(e)
 if not D.enabled or not R.enabled or eq.get_zone_id()~=423 or eq.get_zone_instance_id()~=0 then return end
 local n=e.self;local spawn=n:GetSpawnPointID();local npc=n:GetNPCTypeID()
 if R.wrong_sources[spawn]==npc then remove_piece(n)
 elseif npc==R.maleko_npc and R.maleko_spawns[spawn]then
  -- Normal world rare: no task ownership restriction. Looting and the carried
  -- Collect objective use ordinary native custody, never scripted credit.
  remove_piece(n)
  n:AddItem(R.fire_piece,R.fire_piece_count)
 end
end
return M
