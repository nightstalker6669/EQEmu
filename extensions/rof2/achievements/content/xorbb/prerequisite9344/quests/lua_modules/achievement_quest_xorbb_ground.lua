-- Physical one-shot notes/ham. Native Object::HandleClick owns inventory transfer
-- and Loot credit; this module never grants items or increments an objective.
-- Requires numeric e.object_id in the native pickup callback.
local D=require('achievement_quest_xorbb_data')
local G=require('achievement_quest_xorbb_ground_data')
local M={}
local NS='rof2.xorbb.ground.v1'
local function yes(v)return v==true or v==1 end
local function integer(v)return type(v)=='number'and v==v and v>=0 and v<math.huge and v==math.floor(v)end
local function active(c,g)
 return D.enabled==true and D.ready[g.task]==true and c and c.valid and eq.get_zone_id()==753
  and yes(c:IsTaskActive(g.task))and yes(c:IsTaskActivityActive(g.task,g.activity))
end
local function owned(o,c,key,g)
 return o and o.valid and o:GetEntityVariable(NS)==key
  and o:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())
  and o:GetEntityVariable(NS..'.task')==tostring(g.task)
  and o:GetItemID()==g.item
end
function M.place(c,key)
 local g=G[key]
 if not g or not active(c,g)then return nil,'inactive or unknown ground source'end
 if g.geometry_ready==false then return nil,'elevated note geometry and real ladder placement remain unresolved'end
 -- The caller must first establish the scene prerequisite, notably the actual
 -- climbable ladder for forest_note. Do not call that source from a radius timer.
 for o in eq.get_entity_list():GetObjectList().entries do
  if owned(o,c,key,g)then return o:GetID()end
 end
 local id=eq.create_ground_object(g.item,g.x,g.y,g.z,g.heading,D.ground_decay_ms)
 if not integer(id)or id==0 then return nil,'ground source creation failed'end
 local o=eq.get_entity_list():GetObjectByID(id)
 if not o or not o.valid or o:GetItemID()~=g.item then
  return nil,'created ground identity could not be verified; stop scene rather than retry blindly'
 end
 o:SetEntityVariable(NS,key)
 o:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()))
 o:SetEntityVariable(NS..'.task',tostring(g.task))
 if not owned(o,c,key,g)then
  -- Only this just-created entity is removed; no existing world/player item.
  o:Depop()
  return nil,'ground ownership tag could not be retained'
 end
 return id
end
function M.before_pickup(e)
 if eq.get_zone_id()~=753 or not e.self or not e.self.valid or not e.item or not e.item.valid then return 0 end
 local id=e.item:GetID();local managed_item=false
 for _,g in pairs(G)do if g.item==id then managed_item=true;break end end
 if not managed_item then return 0 end
 if not integer(e.object_id)or e.object_id==0 then
  if D.enabled then e.self:Message(13,'The quest pickup needs the updated server object-identity binding.');return 1 end
  return 0
 end
 local o=eq.get_entity_list():GetObjectByID(e.object_id)
 if not o or not o.valid then return 1 end
 local key=o:GetEntityVariable(NS)
 if key==''then return 0 end -- Preserve ordinary drops and other existing sources.
 local g=G[key]
 if not g or not owned(o,e.self,key,g)or id~=g.item or not active(e.self,g)then return 1 end
 local dx,dy,dz=e.self:GetX()-o:GetX(),e.self:GetY()-o:GetY(),e.self:GetZ()-o:GetZ()
 if dx*dx+dy*dy>D.ground_pickup_xy*D.ground_pickup_xy or math.abs(dz)>D.ground_pickup_z then return 1 end
 if key=='dusty_ham'then require('achievement_quest_xorbb_group').before_dusty_pickup(e.self,o)end
 return 0 -- Native successful inventory placement, then native Loot credit.
end
function M.clear(c,task)
 if eq.get_zone_id()~=753 or not c or not c.valid then return end
 for o in eq.get_entity_list():GetObjectList().entries do
  if o.valid then
   local key=o:GetEntityVariable(NS);local g=G[key]
   if g and(task==nil or task==g.task)and owned(o,c,key,g)then o:Depop()end
  end
 end
end
return M
