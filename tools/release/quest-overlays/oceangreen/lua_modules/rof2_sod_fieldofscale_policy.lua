-- Approved LOCAL FoS policies, exact proposal-r1. Native task membership and
-- actual event providers remain authoritative; no serving installation yet.
local M={}

-- Native shared IDs are uint64 decimal strings. Never round through a Lua
-- double before selection; all members/reloads must choose the same actor.
function M.index(identity,count)
 if type(count)~='number' or count<1 or count%1~=0 or count>100000 then return nil end
 if type(identity)~='string' or not identity:match('^%d+$') or #identity>20 then return nil end
 local canonical=identity:gsub('^0+','')
 if canonical=='' or (#canonical==20 and canonical>'18446744073709551615') then return nil end
 local remainder=0
 for i=1,#canonical do remainder=(remainder*10+canonical:byte(i)-48)%count end
 return remainder+1
end

function M.scholar_success(client,target,actor,run)
 local selected=M.index(run.id,6)
 if not selected or not actor or not actor.index then return nil end
 return actor.role=='scholar' and actor.alive and actor.index==selected
end

-- Actors is the complete pinned list sorted by numeric spawn ID, never the
-- incidental currently loaded NPC enumeration order.
function M.drake_selected(client,npc,actors)
 local selected=M.index(client:GetSharedTaskIdentity(),#actors)
 if not selected then return nil end
 local actor=actors[selected]
 return npc:GetNPCTypeID()==actor.npc and npc:GetSpawnPointID()==actor.spawn
end

function M.wave_target(clients,origin,eligible)
 local selected,distance,character
 for _,client in ipairs(clients) do
  if client and client.valid and client:GetHP()>0 and eligible(client) then
   local dx,dy,dz=client:GetX()-origin[1],client:GetY()-origin[2],client:GetZ()-origin[3]
   local d=dx*dx+dy*dy+dz*dz
   local id=client:CharacterID()
   if not selected or d<distance or (d==distance and id<character) then
    selected,distance,character=client,d,id
   end
  end
 end
 return selected
end

function M.shield_source(client,source,center,half)
 if not client or not client.valid or not source or not source.valid then return false end
 if source:GetID()~=client:GetID() then
  local eye=client:SodFieldOfScaleEye()
  if not eye.valid or eye:GetID()~=source:GetID() then return false end
 end
 return math.abs(source:GetX()-center[1])<=half[1] and
  math.abs(source:GetY()-center[2])<=half[2] and
  math.abs(source:GetZ()-center[3])<=half[3]
end
return M
