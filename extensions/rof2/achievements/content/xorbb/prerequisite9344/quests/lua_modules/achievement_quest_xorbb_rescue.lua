-- Researched intermediate rewards: two recovered rings and one cell key per
-- genuine native Syrkl completion, followed by one ring returned on each hail.
-- Bucket/readback guards expose interrupted custody; not an atomic inventory DB
-- transaction. Never reissue an ambiguous missing/changed partial delivery.
local D=require('achievement_quest_xorbb_data')
local P=require('achievement_quest_xorbb_prison_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local KEY='achievement.xorbb.rescue.v1'
local function yes(v)return v==true or v==1 end
local function ready(c)
 return eq.get_zone_id()==753 and D.enabled==true and D.ready[P.task]==true
  and c and c.valid
end
local function enabled(c)return ready(c)and yes(c:IsTaskActive(P.task))end
local function save(c,v)c:SetBucket(KEY,v);return c:GetBucket(KEY)==v end
local function parse(v)
 local phase,r,k=tostring(v):match('^v1|(%a+)|(%d+)|(%d+)$')
 r,k=tonumber(r),tonumber(k)
 if not phase or not r or not k or r>2147483645 or k>2147483646
  or(phase~='start'and phase~='rings'and phase~='done')then return end
 return phase,r,k
end
local function review(c)
 c:Message(13,'The recovered rings/key delivery needs review. No items were automatically reissued.')
end
function M.claim(c)
 if not ready(c)then return false end
 if yes(c:IsTaskActive(P.task))and not(yes(c:IsTaskActivityActive(P.task,11))
  or yes(c:IsTaskActivityActive(P.task,12))or yes(c:IsTaskActivityActive(P.task,13)))then return false end
 local phase,r,k=parse(c:GetBucket(KEY))
 if not phase then return false end
 if phase=='done'then return true end
 -- The original key is Lore. A repeat completion keeps an already carried key
 -- instead of attempting an impossible duplicate. A banked Lore key may also
 -- be retrieved normally to finish a pending claim; keyrings are untouched.
 local needed_keys=math.max(1,k)
 local rings,keys=I.count(c,66519),I.count(c,66520)
 if phase=='start'then
  if rings==r then
   if not I.give(c,66519,2)then return false end
   rings=I.count(c,66519)
  end
  if rings~=r+2 or(keys~=k and keys~=needed_keys)then review(c);return false end
  if not save(c,'v1|rings|'..r..'|'..k)then review(c);return false end
 end
 if I.count(c,66519)~=r+2 then review(c);return false end
 keys=I.count(c,66520)
 if keys<needed_keys then
  if keys~=k then review(c);return false end
  -- SummonItem normally uses the cursor. If the ring stack is still there,
  -- stop normally; stow it and hail a prisoner to finish the pending key.
  if not I.give(c,66520,1)then return false end
  keys=I.count(c,66520)
 end
 if keys~=needed_keys then review(c);return false end
 if not save(c,'v1|done|'..r..'|'..k)then review(c);return false end
 return true
end
function M.recover(e)
 local n,c=e.self,e.other
 if not n or not n.valid or n:GetNPCTypeID()~=D.giver or not ready(c)
  or yes(c:IsTaskActive(P.task))or not tostring(e.message or ''):lower():find('hail',1,true)then return end
 local phase=parse(c:GetBucket(KEY))
 if phase=='start'or phase=='rings'then
  -- A cancelled task cannot discard an earned, partially delivered reward.
  -- This settles only the existing real-kill marker; it creates no new claim.
  M.claim(c)
 end
end
function M.merit(e)
 local n,c=e.self,e.other
 if not n or not n.valid or n:GetNPCTypeID()~=P.syrkl
  or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 -- Native HandleUpdateTasksOnKill runs before EVENT_KILLED_MERIT. This newly
 -- exposed return stage is the actual completion proof; no synthetic kill.
 if not enabled(c)or not yes(c:IsTaskActivityActive(P.task,11))then return end
 if c:GetBucket(KEY)==''then
  if not save(c,'v1|start|'..I.count(c,66519)..'|'..I.count(c,66520))then return end
 end
 M.claim(c)
end
function M.ring(c,activity)
 if not enabled(c)or not yes(c:IsTaskActivityActive(P.task,activity))then return false end
 local phase=parse(c:GetBucket(KEY))
 if phase~='done'and not M.claim(c)then return false end
 local key=KEY..'.return.'..activity
 local v=c:GetBucket(key)
 if v=='done'then return true end
 local before
 if v==''then
  before=I.count(c,66519)
  if before<1 then c:Message(15,'Bring the recovered ring back first.');return false end
  local marker='pending|'..before;c:SetBucket(key,marker)
  if c:GetBucket(key)~=marker then return false end
 else before=tonumber(v:match('^pending|(%d+)$'))end
 if not before or before<1 or before>2147483647 then review(c);return false end
 local count=I.count(c,66519)
 if count==before and not I.take_one(c,66519)then return false end
 if I.count(c,66519)~=before-1 then review(c);return false end
 c:SetBucket(key,'done');return c:GetBucket(key)=='done'
end
function M.before_update(e)
 if tonumber(e.task_id)~=P.task then return 0 end
 local a=tonumber(e.activity_id)
 if a==12 or a==13 then
  return e.self:GetBucket(KEY..'.return.'..a)=='done'and 0 or 1
 end
 return 0
end
function M.accepted(c,id)
 if id~=P.task or not c or not c.valid then return end
 local old=c:GetBucket(KEY)
 if old~=''then
  local phase=parse(old)
  if phase~='done'then return end -- Preserve unresolved custody for review.
 end
 c:DeleteBucket(KEY)
 c:DeleteBucket(KEY..'.return.12');c:DeleteBucket(KEY..'.return.13')
end
function M.requestable(c,id)
 if id~=P.task then return true end
 local old=c:GetBucket(KEY)
 if old==''then return true end
 local phase=parse(old)
 return phase=='done'
end
return M
