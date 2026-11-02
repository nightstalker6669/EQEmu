-- Whole-family adapter. Native tasks own ordinary kill, loot, collect and delivery credit.
local D=require('achievement_quest_buriedsea_data')
local S=require('buriedsea_scene_native')
local M={}
local function yes(v)return v==true or v==1 end
local function active(c,id)return c and c.valid and yes(c:IsTaskActive(id))end
local function static()return eq.get_zone_instance_id()==0 end
local function inside(c,b)
 return b and c:GetX()>=b.min_x and c:GetX()<=b.max_x and c:GetY()>=b.min_y and c:GetY()<=b.max_y and c:GetZ()>=b.min_z and c:GetZ()<=b.max_z
end
local function carried_slot(c,item)
 -- Personal inventory and cursor only. CountItem/RemoveItem include banks.
 local slot=c:GetInventory():HasItem(item,1,34)
 return slot and slot>=0 and slot or nil
end
function M.eligible(c,t)
 if not D.enabled or not static()or not t or not t.enabled or not t.runtime_dependencies_ready or c:GetLevel()<t.min_level or active(c,t.id)then return false end
 if not t.repeatable and yes(c:IsTaskCompleted(t.id))then return false end
 return yes(eq.is_task_appropriate(t.id))
end
function M.say(e)
 if not D.enabled or not static()then return end
 local ids=D.givers[e.self:GetNPCTypeID()];if not ids then return end
 local message=e.message:lower();local requested=false
 for _,word in ipairs({'hail','task','work','help','willing','errand','supplies','adventurer','island','some things'})do if message:find(word,1,true)then requested=true;break end end
 if not requested then return end
 S.resupply(e.other,e.self:GetNPCTypeID())
 local available={}
 for _,id in ipairs(ids)do if M.eligible(e.other,D.tasks[id])then available[#available+1]=id end end
 if #available>0 then eq.task_selector(available)end
end
function M.accepted(e)
 local t=D.tasks[e.task_id];local c=e.other
 if not D.enabled or not static()or not t or not t.enabled or not t.runtime_dependencies_ready or not active(c,t.id)or e.self:GetNPCTypeID()~=t.giver then return end
 -- Supplies are issued only after a real accepted task, never on selector open.
 -- Bank custody prevents duplicate lore supplies; the activity still requires carried custody.
 if t.supply_item and c:CountItem(t.supply_item)==0 then c:SummonItem(t.supply_item,1)end
 S.accepted(c,t.id)
 M.resume({self=c})
end
function M.before_update(e)
 local t=D.tasks[e.task_id];if not t then return 0 end
 if not D.enabled or not t.enabled or not t.runtime_dependencies_ready or not static()then return 1 end
 return 0
end
function M.resume(e)
 local c=e.self;local needed=S.needs_timer(c)
 if D.enabled and static()then
  for _,id in ipairs(D.ordered_tasks)do local t=D.tasks[id]
   if t.enabled and t.runtime_dependencies_ready and active(c,id)then
    for _,a in ipairs(t.activities)do if a.owner=='geometry'and a.zone==eq.get_zone_id()and yes(c:IsTaskActivityActive(id,a.id))then needed=true;break end end
   end
   if needed then break end
  end
 end
 if needed then c:SetTimer(D.timer,1)else c:StopTimer(D.timer)end
end
function M.timer(e)
 if e.timer~=D.timer then return false end
 local c=e.self
 if not D.enabled or not static()then c:StopTimer(D.timer);return true end
 S.tick(c)
 for _,id in ipairs(D.ordered_tasks)do local t=D.tasks[id]
  if t.enabled and t.runtime_dependencies_ready and active(c,id)then
   for _,a in ipairs(t.activities)do
    if a.owner=='geometry'and a.zone==eq.get_zone_id()and inside(c,a.bounds)and yes(c:IsTaskActivityActive(id,a.id))and
       (not a.requires_visible or(not yes(c:IsInvisible())and c:GetInvisibleUndeadLevel()==0))and
       (not a.requires_item_id or carried_slot(c,a.requires_item_id))then
     c:UpdateTaskActivity(id,a.id,1)
    end
   end
  end
 end
 M.resume(e);return true
end
function M.trade(e)
 if not D.enabled or not static()or not D.givers[e.self:GetNPCTypeID()]then return end
 local c=e.other;local handin,required,items={},{},{}
 for slot=1,4 do
  local item=e.trade['item'..slot]
  if item and item.valid then
   items[#items+1]=item;local id=tostring(item:GetID());local quantity=item:IsStackable()and item:GetCharges()or 1
   handin[id]=(handin[id]or 0)+quantity
   local used=item:GetTaskDeliveredCount();if used>0 then required[id]=(required[id]or 0)+used end
  end
 end
 for _,coin in ipairs({'copper','silver','gold','platinum'})do if(e.trade[coin]or 0)>0 then handin[coin]=e.trade[coin]end end
 local bundles={}
 for _,id in ipairs(D.givers[e.self:GetNPCTypeID()])do local t=D.tasks[id]
  if t.enabled and t.runtime_dependencies_ready and active(c,id)then
   for _,a in ipairs(t.activities)do
    if a.owner=='bundle'and a.zone==eq.get_zone_id()and yes(c:IsTaskActivityActive(id,a.id))then
     local complete=true
     for _,part in ipairs(a.bundle_items)do local key=tostring(part.item_id);if(handin[key]or 0)-(required[key]or 0)<part.count then complete=false;break end end
     if complete then
      for _,part in ipairs(a.bundle_items)do local key=tostring(part.item_id);required[key]=(required[key]or 0)+part.count end
      bundles[#bundles+1]={task=id,activity=a.id}
     end
    end
   end
  end
 end
 -- One custody transaction covers native consumption plus each complete bundle.
 -- Partial bundles, excess and unrelated goods are returned without credit.
 if next(required)and yes(e.self:CheckHandin(c,handin,required,items))then
  for _,b in ipairs(bundles)do if yes(c:IsTaskActivityActive(b.task,b.activity))then c:UpdateTaskActivity(b.task,b.activity,1)end end
 end
 e.self:ReturnHandinItems(c)
end
return M
