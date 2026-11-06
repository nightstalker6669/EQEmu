-- Real native itemless click Objects. Shared nodes stay for other characters;
-- each character/run can credit each of three distinct nodes per color once.
-- That sharing/layout policy is a labelled configurable reconstruction.
local D=require('achievement_quest_xorbb_mercenary_data')
local P=require('achievement_quest_xorbb_krystuls_data')
local M={};local NS='rof2.xorbb.krystuls.v1'
local function yes(v)return v==true or v==1 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<99990 end
local function enabled()return D.enabled==true and D.ready[P.task]==true and P.runtime_capability==true
 and P.models_ready==true and P.geometry_ready==true and eq.get_zone_id()==753
 and type(eq.create_transient_click_object)=='function'and type(eq.remove_transient_click_object)=='function'end
local function owner()return NS..'.zone.'..eq.get_zone_instance_id()end
local function run(c)return c:GetBucket('rof2.xorbb.mercenary.run.'..P.task)end
local function valid_run(r)return type(r)=='string'and r:match('^[1-9]%d*$')~=nil end
local function node_info(key)
 for _,color in ipairs(P.colors)do for i=1,3 do if key==color.key..'.'..i then return color,i end end end
end
local function objects()
 local found={};local duplicate=false
 for o in eq.get_entity_list():GetObjectList().entries do
  if o.valid and o:GetDBID()==0 and o:GetEntityVariable(NS..'.owner')==owner()then
   local key=o:GetEntityVariable(NS..'.node');local color=node_info(key)
   if color and o:GetEntityVariable(NS..'.token')~=''and o:GetModelName()==color.model then
    if found[key]then duplicate=true else found[key]=o end
   else
    duplicate=true -- tagged but malformed/changed nodes must not be replaced blindly
   end
  end
 end
 return found,duplicate
end
local function remove(o)
 if o and o.valid and o:GetDBID()==0 and o:GetEntityVariable(NS..'.owner')==owner()then
  return eq.remove_transient_click_object(owner(),o:GetID(),o:GetEntityVariable(NS..'.token'))
 end
 return false
end
function M.init(giver)
 if not enabled()or not giver or not giver.valid or giver:GetNPCTypeID()~=P.giver or giver:GetSpawnPointID()<=0 then return false end
 if #P.colors~=4 or #P.offsets~=3 or not finite(P.max_floor_delta)or P.max_floor_delta<=0
  or not finite(P.max_cluster_step)or P.max_cluster_step<=0 then return false end
 local existing,duplicate=objects();if duplicate then eq.debug('Krystul owned-node identity duplicate; no creation.');return false end
 local missing={}
 -- Resolve all missing positions before any creation. FindGroundZ third
 -- argument is a relative Z offset, so region.z - giver.z selects the region.
 for _,color in ipairs(P.colors)do
  if not(finite(color.x)and finite(color.y)and finite(color.z))then return false end
  local base=giver:FindGroundZ(color.x,color.y,color.z-giver:GetZ())
  if not finite(base)or math.abs(base-color.z)>P.max_floor_delta then return false end
  for i,offset in ipairs(P.offsets)do
   if not finite(offset.x)or not finite(offset.y)or offset.x^2+offset.y^2>100 then return false end
   local key=color.key..'.'..i
   if not existing[key]then
    local x,y=color.x+offset.x,color.y+offset.y
    local z=giver:FindGroundZ(x,y,color.z-giver:GetZ())
    if not finite(z)or math.abs(z-base)>P.max_cluster_step then return false end
    missing[#missing+1]={key=key,color=color,x=x,y=y,z=z+0.1}
   end
  end
 end
 local made={}
 for _,p in ipairs(missing)do
  local h=eq.create_transient_click_object(owner(),p.color.model,p.x,p.y,p.z,0)
  if not h or type(h.entity_id)~='number'or type(h.token)~='string'or h.token==''then
   for _,o in ipairs(made)do remove(o)end;return false
  end
  local o=eq.get_entity_list():GetObjectByID(h.entity_id)
  if not o or not o.valid or o:GetID()~=h.entity_id or o:GetDBID()~=0 or o:GetModelName()~=p.color.model then
   -- Use only the returned exact helper lease, never an arbitrary object Depop.
   eq.remove_transient_click_object(owner(),h.entity_id,h.token)
   for _,prior in ipairs(made)do remove(prior)end;return false
  end
  o:SetEntityVariable(NS..'.owner',owner());o:SetEntityVariable(NS..'.node',p.key)
  o:SetEntityVariable(NS..'.token',h.token);made[#made+1]=o
 end
 giver:SetTimer(P.timer,P.interval_seconds);return true
end
function M.enter(e)
 if not enabled()then return end
 local giver
 for n in eq.get_entity_list():GetNPCList().entries do
  if n.valid and n:GetNPCTypeID()==P.giver and n:GetSpawnPointID()>0 then
   if giver then eq.debug('Multiple original Gronk sources; no node creation.');return end;giver=n
  end
 end
 if giver then M.init(giver)end
end
function M.timer(e)
 if e.timer~=P.timer or not e.self or not e.self.valid or e.self:GetNPCTypeID()~=P.giver then return end
 -- No automatic creation/retry on failure. Startup or an enter event owns init.
 if not enabled()then
  e.self:StopTimer(P.timer)
  if type(eq.remove_transient_click_object)=='function'then
   local own=objects();for _,o in pairs(own)do remove(o)end
  end
 end
end
function M.click(e)
 local c,o=e.self,e.object
 if not enabled()or not c or not c.valid or not c:IsClient()or not o or not o.valid
  or type(e.object_id)~='number'or e.object_id<=0 or o:GetID()~=e.object_id or o:GetDBID()~=0
  or o:GetEntityVariable(NS..'.owner')~=owner()then return end
 local key=o:GetEntityVariable(NS..'.node');local color=node_info(key)
 if not color or o:GetModelName()~=color.model or o:GetEntityVariable(NS..'.token')==''then return end
 local owned,duplicate=objects()
 -- Re-read the live ID and its original lease tag; reused IDs/foreign objects
 -- cannot inherit a recorded node. Callback wrapper is never retained.
 local live=eq.get_entity_list():GetObjectByID(e.object_id)
 if duplicate or not owned[key]or owned[key]:GetID()~=e.object_id or not live or not live.valid
  or live:GetEntityVariable(NS..'.token')~=o:GetEntityVariable(NS..'.token')then return end
 if not yes(c:IsTaskActive(P.task))or not yes(c:IsTaskActivityActive(P.task,color.activity))then return end
 local epoch=run(c);if not valid_run(epoch)then return end
 if not finite(P.click_range)or P.click_range<=0 or P.click_range>100 then return end
 local dx,dy,dz=c:GetX()-o:GetX(),c:GetY()-o:GetY(),c:GetZ()-o:GetZ()
 if dx*dx+dy*dy+dz*dz>P.click_range^2 or not c:CheckLoSToLoc(o:GetX(),o:GetY(),o:GetZ()+0.1)then return end
 local bucket=NS..'.node.'..key;local value=c:GetBucket(bucket)
 if value==epoch..':pending'or value==epoch..':done'then return end
 local before=tonumber(eq.get_task_activity_done_count(P.task,color.activity))
 if not before or before~=math.floor(before)or before<0 or before>=3 then return end
 c:SetBucket(bucket,epoch..':pending');if c:GetBucket(bucket)~=epoch..':pending'then return end
 c:SetEntityVariable(NS..'.witness',tostring(color.activity)..':'..epoch)
 local ok=pcall(function()c:UpdateTaskActivity(P.task,color.activity,1)end)
 c:SetEntityVariable(NS..'.witness','')
 local after=tonumber(eq.get_task_activity_done_count(P.task,color.activity))
 if ok and after==before+1 then c:SetBucket(bucket,epoch..':done')
 else eq.debug('Krystul progress ambiguous; pending node preserved without automatic replay.')end
end
function M.before_update(e)
 if tonumber(e.task_id)~=P.task then return 0 end
 local a=tonumber(e.activity_id)
 if not a or a~=math.floor(a)or a<0 or a>3 then return 0 end
 local c=e.self
 if not enabled()or not c or not c.valid then return 1 end
 local proof=c:GetEntityVariable(NS..'.witness');c:SetEntityVariable(NS..'.witness','')
 return valid_run(run(c))and proof==tostring(a)..':'..run(c)and 0 or 1
end
return M
