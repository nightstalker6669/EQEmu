-- Native shared-task offers and occurrence-bound travel. Instance creation is
-- owned by tasks.dz_template_id in world; no quest creation retry or journal.
local D=require('achievement_quest_xorbb_mission_data')
local M={}
local function yes(v)return v==true or v==1 end
local function integer(v)return type(v)=='number'and v==math.floor(v)and v>0 and v<=2147483647 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<math.huge end
local function location(p)
 if type(p)~='table'or p.zone~=D.zone then return false end
 for _,k in ipairs({'x','y','z','h'})do if not finite(p[k])then return false end end
 return true
end
local function configured(d)
 return D.enabled==true and d.ready==true and d.geometry_ready==true
  and integer(d.task)and integer(d.template)and integer(d.version)
  and location(d.entry)and location(d.safereturn)
end
local function client(c)return c and c.valid and c:IsClient()end
local function public()return eq.get_zone_id()==D.zone and eq.get_zone_instance_id()==0 end
local function phrase(text,p)return (' '..tostring(text or ''):lower()..' '):find('%f[%a]'..p..'%f[%A]')~=nil end
local function eligible(c,d)
 if c:GetLevel()<D.minimum_level then return false end
 for _,id in ipairs(d.prerequisites)do
  if type(id)=='string'then id=D.missions[id]and D.missions[id].task end
  if not integer(id)or not yes(c:IsTaskCompleted(id))then return false end
 end
 return true
end
local function identity(c)
 local id=c:GetSharedTaskIdentity()
 if type(id)=='string'and id:match('^[1-9]%d*$')then return id end
end
local function instance(c,d)
 if not yes(c:IsTaskActive(d.task))or not identity(c)then return nil end
 local id=c:GetSharedTaskInstanceID(d.task,D.zone,d.version)
 if integer(id)and id<=65535 then return id end
end
local function message(c,text)c:Message(13,text)end
function M.say(e)
 if not public()or not client(e.other)or not e.self or not e.self.valid then return false end
 local c=e.other
 for _,d in pairs(D.missions)do
  if configured(d)and e.self:GetNPCTypeID()==d.giver then
   if phrase(e.message,d.request)then
    if not eligible(c,d)then message(c,'Complete the preceding work before requesting this mission.');return true end
    -- Native world admission owns group size, membership, eligibility and timers.
    -- Selecting is not acceptance and must not initialize an occurrence here.
    if not yes(c:IsTaskActive(d.task))and yes(eq.is_task_appropriate(d.task))then eq.task_selector({d.task})end
    return true
   elseif phrase(e.message,d.enter)then
    local id=instance(c,d)
    if not id then message(c,'Your active mission instance is not ready for entry.');return true end
    local p=d.entry
    c:MovePCInstance(D.zone,id,p.x,p.y,p.z,p.h)
    -- Travel submission is not arrival, objective completion or native acceptance.
    return true
   end
  end
 end
 return false
end
-- Native world admission loads the configured task DZ template once. There is
-- deliberately no TASK_ACCEPTED creation handler: an asynchronous zero mapping
-- is pending/unavailable, not authority to request another dynamic zone.
return M
