-- Actual ladder-item interaction, owned transient geometry and physical note.
-- Never use legacy create_door: that path replaces existing zone door instances.
local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_ladder_data')
local N=require('achievement_quest_xorbb_ground_data').forest_note
local G=require('achievement_quest_xorbb_ground')
local M={}
local NS='rof2.xorbb.ladder.v1'
local function yes(v)return v==true or v==1 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<math.huge end
local function owner(c)
 return NS..':'..eq.get_zone_id()..':'..eq.get_zone_instance_id()..':'..c:CharacterID()
end
local function active(c)
 return c and c.valid and eq.get_zone_id()==753 and D.enabled==true
  and D.ready[3040474]==true and yes(c:IsTaskActive(3040474))
  and yes(c:IsTaskActivityActive(3040474,7))
end
local function pose_ready()
 if P.geometry_ready~=true or N.geometry_ready~=true then return false end
 for _,k in ipairs({'x','y','z','use_x','use_y','use_z','heading','incline','open_type','size'})do
  if not finite(P[k])then return false end
 end
 return P.open_type==math.floor(P.open_type)and P.open_type>=0 and P.open_type<=255
  and P.size==math.floor(P.size)and P.size>=1 and P.size<=65535
  and P.incline==math.floor(P.incline)and P.incline>=0 and P.incline<=4294967295
end
local function tree_ready()
 local t=P.tree
 if type(t)~='table'then return false end
 local o=eq.get_entity_list():GetDoorsByDoorID(t.native_door_id)
 if not o or not o.valid or o:GetDoorName()~=t.model or o:GetDoorID()~=t.native_door_id
  or o:GetSize()~=t.size or o:GetOpenType()~=t.open_type or o:GetIncline()~=t.incline then return false end
 local tol=t.tolerance
 return math.abs(o:GetX()-t.x)<=tol and math.abs(o:GetY()-t.y)<=tol
  and math.abs(o:GetZ()-t.z)<=tol and math.abs(o:GetHeading()-t.heading)<=tol
end
local function forget(c)
 c:SetEntityVariable(NS..'.id','');c:SetEntityVariable(NS..'.token','')
end
function M.clear(c)
 if not c or not c.valid or eq.get_zone_id()~=753 then return false end
 local id=tonumber(c:GetEntityVariable(NS..'.id'))
 local token=c:GetEntityVariable(NS..'.token')
 if not id and token==''then return true end
 if not id or token==''then return false end
 -- Retain the handle if the required binding is unavailable. Never fall back
 -- to removing a model, DBID0, a client door ID or an unrelated recycled ID.
 if type(eq.remove_transient_door)~='function'then return false end
 local removed=eq.remove_transient_door(owner(c),id,token)
 forget(c) -- A stale nonce cannot authorize removal of a replacement entity.
 return removed==true
end
local function fail(c,text)
 c:SetEntityVariable(NS..'.failure',text)
 c:Message(13,text)
 return 1
end
function M.click(e)
 if tonumber(e.item_id)~=66508 then return 0 end
 local c=e.self
 if not active(c)then return 0 end
 local slot=tonumber(e.slot_id)
 if not slot or c:GetItemIDAt(slot)~=66508 then return 1 end
 if c:GetEntityVariable(NS..'.failure')~=''then return 1 end
 if not pose_ready()then
  return fail(c,'The elevated note and ladder climb route are not configured yet.')
 end
 if type(eq.create_transient_door)~='function'or type(eq.remove_transient_door)~='function'then
  return fail(c,'The owned quest-door server binding is required for this ladder.')
 end
 if not tree_ready()then
  return fail(c,'The configured quest tree could not be verified. Please report this before retrying.')
 end
 -- The interaction is at the ladder's feet. Its off-centre model pivot may be
 -- far above the player, especially when the original model is scaled.
 local dx,dy,dz=c:GetX()-P.use_x,c:GetY()-P.use_y,c:GetZ()-P.use_z
 if dx*dx+dy*dy>D.door_interaction_xy^2 or math.abs(dz)>D.door_interaction_z then
  c:Message(15,'Bring the ladder to the tree before using it.');return 1
 end
 -- Handle state lives on the client, so ordinary Lua module reloads do not
 -- discard the nonce and create duplicate scene doors.
 -- There is no public nonce-inspection API. Reconcile a repeat click by exact
 -- owned removal before recreating, rather than trusting a recycled entity ID.
 if not M.clear(c)then
  return fail(c,'The previous quest ladder handle could not be reconciled. Please report this before retrying.')
 end
 do
  local h=eq.create_transient_door(owner(c),P.model,P.x,P.y,P.z,P.heading,P.open_type,P.size)
  if type(h)~='table'or not finite(h.entity_id)or h.entity_id<=0 or h.entity_id~=math.floor(h.entity_id)
   or type(h.token)~='string'or h.token==''then
   return fail(c,'The quest ladder could not be created. Please report this before retrying.')
  end
  c:SetEntityVariable(NS..'.id',tostring(h.entity_id))
  c:SetEntityVariable(NS..'.token',h.token)
  -- Resolve only the just-created native entity, never a client door ID/model
  -- search. Existing setters rebuild the client door list but do not replace
  -- the server's door entities or invalidate the owned removal nonce.
  local door=eq.get_entity_list():GetDoorsByID(h.entity_id)
  -- Lua_Door:GetID reads Entity::id, which AddDoor does not initialize;
  -- GetDoorsByID itself indexes the separately assigned Doors::m_entity_id.
  if not door or not door.valid or door:GetDoorDBID()~=0 or door:GetDoorName()~=P.model then
   M.clear(c)
   return fail(c,'The owned ladder entity could not be verified before setting its pose.')
  end
  local ok=pcall(function()door:SetIncline(P.incline)end)
  if not ok or door:GetIncline()~=P.incline then
   M.clear(c)
   return fail(c,'The owned ladder incline could not be verified. Please report this before retrying.')
  end
 end
 -- A successful item request is not climb/loot credit. Only a real elevated
 -- ground object and subsequent native inventory pickup advance the objective.
 local note,why=G.place(c,'forest_note')
 if not note then
  M.clear(c)
  return fail(c,'The elevated quest note could not be created: '..tostring(why))
 end
 return 1
end
function M.stage(e)
 -- Keep the real descent route after note pickup and during the bat fight.
 -- Completing Loot must not remove the ladder underneath the player.
 if tonumber(e.task_id)==3040474 and e.self and e.self.valid
  and not yes(e.self:IsTaskActive(3040474))then M.clear(e.self)end
end
function M.closed(e)
 if tonumber(e.task_id)==3040474 then M.clear(e.self)end
end
function M.accepted(c,id)
 if id~=3040474 or not c or not c.valid then return end
 M.clear(c);c:SetEntityVariable(NS..'.failure','')
end
return M
