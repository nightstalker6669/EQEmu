-- Researched action phrases and Grolak's actual Goblin/sitting requirements.
-- Concise authored guidance; no copied quest dialogue or scripted rewards.
local D=require('achievement_quest_xorbb_mercenary_data')
local S=require('achievement_quest_xorbb_mercenary_supplies')
local M={};local NS='rof2.xorbb.mercenary.dialogue.v1'
local function yes(v)return v==true or v==1 end
local function active(c,t,a)return D.enabled and D.ready[t]==true and eq.get_zone_id()==753
 and c and c.valid and yes(c:IsTaskActive(t))and yes(c:IsTaskActivityActive(t,a))end
local function word(message,phrase)return tostring(message or ''):lower():find('%f[%a]'..phrase..'%f[%A]')~=nil end
local prisoners={ [753033]=true,[753045]=true,[753050]=true,[753056]=true,[753059]=true,[753077]=true,[753085]=true,[753087]=true }
function M.say(e)
 local n,c=e.self,e.other
 if not n or not n.valid or not c or not c.valid or not D.enabled or eq.get_zone_id()~=753 then return false end
 local id=n:GetNPCTypeID();local text=e.message or ''
 if id==753114 then
  for _,p in ipairs({{3,'fun'},{6,'waste'}})do
   if active(c,3040484,p[1])and word(text,p[2])then
    c:UpdateTaskActivity(3040484,p[1],1);S.recover(c,id);return true
   end
  end
 elseif prisoners[id]and active(c,3040485,0)and word(text,'idea')then
  c:UpdateTaskActivity(3040485,0,1);return true
 elseif id==753030 then
  -- An actual Goblin hail is the proof for this SpeakWith objective. Scope
  -- its witness to this one native update call; never leave a stale permission.
  c:SetEntityVariable(NS..'.goblin_hail','')
  if tonumber(e.language)==14 and word(text,'hail')then
   for _,a in ipairs({7,12})do if active(c,3040485,a)then
    c:SetEntityVariable(NS..'.goblin_hail',tostring(a))
    local ok=pcall(function()c:UpdateTaskActivity(3040485,a,1)end)
    c:SetEntityVariable(NS..'.goblin_hail','')
    if not ok then eq.debug('Xorbb Goblin hail update failed; no stale witness retained.')end
    return true
   end end
  end
  if tonumber(e.language)~=14 then return false end
  if active(c,3040485,1)and word(text,'lightning')then
   c:UpdateTaskActivity(3040485,1,1);S.recover(c,id);return true
  end
  if active(c,3040485,8)and word(text,'started')and yes(c:IsSitting())then
   local run=c:GetBucket('rof2.xorbb.mercenary.run.3040485')
   if not tostring(run):match('^[1-9]%d*$')then return true end
   -- This grants access to a replacement implement, never the combined result.
   c:SetBucket('rof2.xorbb.mercenary.coffin_run',run)
   if c:GetBucket('rof2.xorbb.mercenary.coffin_run')==run then S.recover(c,id)end
   return true
  end
 elseif id==753284 then
  for _,p in ipairs({{4,'protect'},{6,'robe'},{9,'scared of heights'}})do
   if active(c,3040486,p[1])and word(text,p[2])then
    c:UpdateTaskActivity(3040486,p[1],1)
    if p[1]==6 then require('achievement_quest_xorbb_gronk').arm(n)end
    return true
   end
  end
 end
 return false
end
local function coffin_permission(c)
 if not c or not c.valid then return false end
 local epoch=c:GetBucket('rof2.xorbb.mercenary.run.3040485')
 return type(epoch)=='string'and epoch:match('^[1-9]%d*$')~=nil
  and c:GetBucket('rof2.xorbb.mercenary.coffin_run')==epoch
end
function M.validate(e)
 if tonumber(e.recipe_id)~=3040485 then return 0 end
 -- Native COMBINE_VALIDATE runs before ingredient consumption. A retained
 -- coffin from an older run is not current Grolak instruction permission.
 if not active(e.self,3040485,8)or not coffin_permission(e.self)then
  if e.self and e.self.valid then e.self:Message(13,"Follow Grolak's current assembly instructions before combining the golem.")end
  return 1
 end
 return 0
end
function M.before_update(e)
 if tonumber(e.task_id)~=3040485 then return 0 end
 local a=tonumber(e.activity_id)
 if a==8 then return active(e.self,3040485,8)and coffin_permission(e.self)and 0 or 1 end
 if a~=7 and a~=12 then return 0 end
 local c=e.self
 if not active(c,3040485,a)then return 1 end
 local proof=c:GetEntityVariable(NS..'.goblin_hail')
 c:SetEntityVariable(NS..'.goblin_hail','')
 return proof==tostring(a)and 0 or 1
end
return M
